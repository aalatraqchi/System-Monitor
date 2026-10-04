param([string]$Dotnet = '', [string]$OutputDirectory = '')
$ErrorActionPreference = 'Stop'
$env:DOTNET_CLI_TELEMETRY_OPTOUT = '1'
if (!$Dotnet) {
    $localSdk = Join-Path $PSScriptRoot 'build-tools\dotnet\dotnet.exe'
    if (Test-Path -LiteralPath $localSdk) { $Dotnet = $localSdk }
    else { $Dotnet = (Get-Command dotnet -ErrorAction Stop).Source }
}
if (!$OutputDirectory) { $OutputDirectory = Join-Path $PSScriptRoot 'build\temperature-helper' }
$project = Join-Path $PSScriptRoot 'helpers\TemperatureHelper\TemperatureHelper.csproj'
# Ship the helper's .NET runtime locally; users do not need to install .NET to run it.
& $Dotnet publish $project -c Release -r win-x64 --self-contained true -o $OutputDirectory -p:RestoreLockedMode=true
if ($LASTEXITCODE -ne 0) { throw 'Temperature helper build failed. A .NET 8 SDK is required.' }
& (Join-Path $OutputDirectory 'TemperatureHelper.exe') --self-test
if ($LASTEXITCODE -ne 0) { throw 'Temperature helper sensor-selection checks failed.' }
