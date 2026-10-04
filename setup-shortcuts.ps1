# Creates personal shortcuts to this build; Windows will show its normal UAC prompt.
$ErrorActionPreference = 'Stop'
$executable = Join-Path $PSScriptRoot 'build\SystemMonitor.exe'
if (-not (Test-Path -LiteralPath $executable)) { throw 'Run build.ps1 first.' }
$shell = New-Object -ComObject WScript.Shell
$folders = @([Environment]::GetFolderPath('Desktop'), [Environment]::GetFolderPath('Programs'))
foreach ($folder in $folders) {
    $path = Join-Path $folder 'System Monitor.lnk'
    $shortcut = $shell.CreateShortcut($path)
    if ((Test-Path -LiteralPath $path) -and $shortcut.TargetPath -ne $executable) {
        throw "An unrelated shortcut already exists: $path"
    }
    $shortcut.TargetPath = $executable
    $shortcut.WorkingDirectory = Split-Path -Parent $executable
    $shortcut.IconLocation = "$executable,0"
    $shortcut.Description = 'Live CPU, RAM, GPU and temperature monitoring'
    $shortcut.Save()

    # Shell Link's RunAsUser bit is the shortcut's "Run as administrator" option.
    # Preserve all other flags. This requests elevation; it does not bypass UAC.
    $bytes = [IO.File]::ReadAllBytes($path)
    if ($bytes.Length -lt 76 -or [BitConverter]::ToUInt32($bytes, 0) -ne 76) {
        throw "Unexpected shortcut header: $path"
    }
    $flags = [BitConverter]::ToUInt32($bytes, 20) -bor 0x2000
    [BitConverter]::GetBytes([uint32]$flags).CopyTo($bytes, 20)
    [IO.File]::WriteAllBytes($path, $bytes)
    if (([BitConverter]::ToUInt32([IO.File]::ReadAllBytes($path), 20) -band 0x2000) -eq 0) {
        throw "Administrator option was not saved: $path"
    }
    Write-Host "Created administrator shortcut: $path"
}
# Pinning is a separate Windows Start menu action, performed through its UI.
