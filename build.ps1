param(
    [string]$QtRoot = 'C:\Qt',
    [string]$QtVersion = '',
    [string]$MinGW = ''
)
$ErrorActionPreference = 'Stop'

# Default to the newest Qt MinGW kit and toolchain installed under $QtRoot.
if (!$QtVersion) {
    $QtVersion = Get-ChildItem -LiteralPath $QtRoot -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' -and (Test-Path (Join-Path $_.FullName 'mingw_64')) } |
        Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1 -ExpandProperty Name
}
if (!$QtVersion) { throw "No Qt MinGW kit found under $QtRoot. Pass -QtRoot or -QtVersion." }
$qtPackage = Join-Path $QtRoot "$QtVersion\mingw_64"
if (!$MinGW) {
    # Qt records the GCC version it was built with, e.g. 13.1.0 for Tools\mingw1310_64.
    $qconfig = Join-Path $qtPackage 'mkspecs\qconfig.pri'
    $gcc = if (Test-Path -LiteralPath $qconfig) { Get-Content -LiteralPath $qconfig } else { @() }
    $parts = 'MAJOR', 'MINOR', 'PATCH' | ForEach-Object {
        $line = $gcc | Select-String "^QT_GCC_${_}_VERSION\s*=\s*(\d+)" | Select-Object -First 1
        if ($line) { $line.Matches[0].Groups[1].Value }
    }
    if (@($parts).Count -eq 3) { $MinGW = "mingw$($parts -join '')_64" }
    else {
        $MinGW = Get-ChildItem -LiteralPath (Join-Path $QtRoot 'Tools') -Directory -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -match '^mingw(\d+)_64$' } |
            Sort-Object { [int]($_.Name -replace '^mingw(\d+)_64$', '$1') } -Descending |
            Select-Object -First 1 -ExpandProperty Name
    }
}

# Use Qt's matching compiler for this build instead of any other compiler on PATH.
$compilerBin = Join-Path $QtRoot "Tools\$MinGW\bin"
$cmake = Join-Path $QtRoot 'Tools\CMake_64\bin\cmake.exe'
$ninja = Join-Path $QtRoot 'Tools\Ninja\ninja.exe'
$buildDir = Join-Path $PSScriptRoot 'build'
foreach ($required in $qtPackage, (Join-Path $compilerBin 'g++.exe'), $cmake, $ninja) {
    if (!(Test-Path -LiteralPath $required)) {
        throw "Missing $required. Check -QtRoot, -QtVersion and -MinGW, and install CMake and Ninja with the Qt Maintenance Tool."
    }
}
Write-Host "Using Qt $QtVersion (mingw_64) with $MinGW from $QtRoot"
$originalPath = $env:PATH
try {
    $env:PATH = "$compilerBin;$(Join-Path $qtPackage 'bin');$env:PATH"
    & $cmake -S $PSScriptRoot -B $buildDir -G Ninja `
        "-DCMAKE_PREFIX_PATH=$qtPackage" `
        "-DCMAKE_CXX_COMPILER=$(Join-Path $compilerBin 'g++.exe')" `
        "-DCMAKE_MAKE_PROGRAM=$ninja" '-DCMAKE_BUILD_TYPE=Release'
    if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
    & $cmake --build $buildDir
    if ($LASTEXITCODE -ne 0) { throw 'Compilation failed.' }

    # Copy Qt libraries and plugins so the executable runs outside the editor.
    & (Join-Path $qtPackage 'bin\windeployqt.exe') --release --no-translations `
        (Join-Path $buildDir 'SystemMonitor.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Qt deployment failed.' }
}
finally {
    $env:PATH = $originalPath
}
