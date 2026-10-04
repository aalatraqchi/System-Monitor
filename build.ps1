param([string]$QtRoot = 'E:\QTCreator')
$ErrorActionPreference = 'Stop'

# Use Qt's matching compiler for this build instead of the global MSYS2 compiler.
$qtPackage = Join-Path $QtRoot '6.12.0\mingw_64'
$compilerBin = Join-Path $QtRoot 'Tools\mingw1310_64\bin'
$cmake = Join-Path $QtRoot 'Tools\CMake_64\bin\cmake.exe'
$ninja = Join-Path $QtRoot 'Tools\Ninja\ninja.exe'
$buildDir = Join-Path $PSScriptRoot 'build'
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
