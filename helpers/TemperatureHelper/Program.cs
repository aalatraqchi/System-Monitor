using System.Text.Json;
using LibreHardwareMonitor.Hardware;

// C++ requests a snapshot over stdin; one bounded JSON line comes back on stdout.
// Only CPU/GPU monitoring is enabled. The helper never installs a driver or elevates.
internal static class Program
{
    private sealed record Reading(string name, string sensor, float? celsius, string reason);

    private static (string name, float value)? SelectSensor(
        IEnumerable<(string name, float? value)> sensors, bool cpu)
    {
        var valid = sensors.Where(s => s.value is float v && float.IsFinite(v) && v >= 0 && v <= 150);
        // Prefer CPU package (Intel) or Tctl/Tdie (AMD); never label a core as package.
        var preferred = valid.Where(s => cpu
            ? s.name.Equals("CPU Package", StringComparison.OrdinalIgnoreCase)
              || s.name.Contains("Tctl/Tdie", StringComparison.OrdinalIgnoreCase)
            : s.name.Equals("GPU Core", StringComparison.OrdinalIgnoreCase));
        if (!preferred.Any()) preferred = cpu
            ? valid.Where(s => s.name.StartsWith("CPU Core", StringComparison.OrdinalIgnoreCase))
            : Enumerable.Empty<(string name, float? value)>();
        var choice = preferred.OrderByDescending(s => s.value).FirstOrDefault();
        return choice.value.HasValue ? (choice.name, choice.value.Value) : null;
    }

    private static Reading Read(IHardware hardware, bool cpu)
    {
        try
        {
            hardware.Update();
            var choice = SelectSensor(hardware.Sensors.Where(s => s.SensorType == SensorType.Temperature)
                .Select(s => (s.Name, s.Value)), cpu);
            return new(hardware.Name, choice?.name ?? "", choice?.value,
                choice.HasValue ? "" : cpu
                    ? "CPU sensor unavailable; PawnIO driver or administrator access may be required"
                    : "GPU core temperature sensor unavailable");
        }
        catch (Exception)
        {
            return new(hardware.Name, "", null, "Sensor read failed; retrying on the next sample");
        }
    }

    private static int Main(string[] args)
    {
        if (args.Contains("--self-test")) return SelfTest();
        if (!args.Contains("--stdio") && !args.Contains("--once")) return 2;
        var computer = new Computer { IsCpuEnabled = true, IsGpuEnabled = true };
        string message = "";
        try { computer.Open(); }
        catch (Exception) { message = "Sensor initialization failed; readings may be unavailable"; }
        try
        {
            while (args.Contains("--once") || Console.ReadLine() is string command && command == "sample")
            {
                var cpu = new List<Reading>();
                var gpu = new List<Reading>();
                foreach (var hardware in computer.Hardware)
                {
                    if (hardware.HardwareType == HardwareType.Cpu) cpu.Add(Read(hardware, true));
                    else if (hardware.HardwareType is HardwareType.GpuNvidia or HardwareType.GpuAmd or HardwareType.GpuIntel)
                        gpu.Add(Read(hardware, false));
                }
                Console.WriteLine(JsonSerializer.Serialize(new { version = 1, cpu, gpu, message }));
                Console.Out.Flush();
                if (args.Contains("--once")) break;
            }
        }
        finally { computer.Close(); }
        return 0;
    }

    private static int SelfTest()
    {
        void Require(bool value) { if (!value) throw new Exception("Temperature sensor selection failed"); }
        Require(SelectSensor(new[] { ("CPU Core #1", (float?)70), ("CPU Package", (float?)55) }, true)?.value == 55);
        Require(SelectSensor(new[] { ("CPU (Tctl/Tdie)", (float?)62) }, true)?.value == 62);
        Require(SelectSensor(new[] { ("CPU Core #1", (float?)40), ("CPU Core #2", (float?)60) }, true)?.value == 60);
        Require(SelectSensor(new[] { ("GPU Hot Spot", (float?)80), ("GPU Core", (float?)50) }, false)?.value == 50);
        Require(SelectSensor(new[] { ("GPU Memory", (float?)70) }, false) == null);
        Require(SelectSensor(new[] { ("CPU Package", (float?)float.NaN), ("CPU Core #1", (float?)200) }, true) == null);
        Console.WriteLine("CPU package/core and GPU core sensor selection passed");
        return 0;
    }
}
