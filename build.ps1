# FreeDaw Build Script
# Usage:
#   .\build.ps1           - incremental build
#   .\build.ps1 -Clean    - full reconfigure + build

param(
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
$cmake  = "c:\qt\Tools\CMake_64\bin\cmake.exe"
$qtPath = "c:/qt/6.10.2/msvc2022_64"
$ninja  = "c:/qt/Tools/Ninja/ninja.exe"

# Kill running instance if needed
$proc = Get-Process -Name FreeDaw -ErrorAction SilentlyContinue
if ($proc) {
    Write-Host "Stopping running FreeDaw..." -ForegroundColor Yellow
    taskkill /IM FreeDaw.exe /F 2>$null
    Start-Sleep -Milliseconds 500
}

if ($Clean -or !(Test-Path "build\build.ninja")) {
    Write-Host "`n=== Configuring ===" -ForegroundColor Cyan
    $configCmd = "`"$vcvars`" x64 && `"$cmake`" -B build -G Ninja -DCMAKE_PREFIX_PATH=$qtPath -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -DCMAKE_MAKE_PROGRAM=$ninja 2>&1"
    cmd /c $configCmd
    if ($LASTEXITCODE -ne 0) {
        Write-Host "`nConfigure FAILED" -ForegroundColor Red
        exit 1
    }
}

Write-Host "`n=== Building ===" -ForegroundColor Cyan
$buildCmd = "`"$vcvars`" x64 && `"$cmake`" --build build 2>&1"
cmd /c $buildCmd
if ($LASTEXITCODE -ne 0) {
    Write-Host "`nBuild FAILED" -ForegroundColor Red
    exit 1
}

Write-Host "`nBuild OK  ->  build\FreeDaw_artefacts\Debug\FreeDaw.exe" -ForegroundColor Green
