# scripts/check-cross-platform.ps1

param(
    [ValidateSet("windows-ninja-msvc", "linux-ninja-debug")]
    [string]$Preset = "",
    [switch]$Clean,
    [switch]$SmokeOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$Root = Split-Path $PSScriptRoot

if (-not $Preset) {
    if ($IsWindows -or $env:OS -eq "Windows_NT") {
        $Preset = "windows-ninja-msvc"
    } else {
        $Preset = "linux-ninja-debug"
    }
}

$BuildDir = switch ($Preset) {
    "windows-ninja-msvc" { Join-Path $Root "build/windows-ninja-msvc" }
    "linux-ninja-debug" { Join-Path $Root "build/linux-ninja-debug" }
}

Write-Host "TACHYON cross-platform check" -ForegroundColor Cyan
Write-Host "Preset: $Preset"
Write-Host "BuildDir: $BuildDir"

if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "Removing build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BuildDir
}

if ($env:OS -eq "Windows_NT") {
    $global:LASTEXITCODE = 0
    & "$Root\scripts\Enable-DevTools.ps1"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    $global:LASTEXITCODE = 0
    & "$Root\scripts\enable-vs-env.ps1"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Get-Process TachyonTests -ErrorAction SilentlyContinue | Stop-Process -Force
    Get-Process tachyon -ErrorAction SilentlyContinue | Stop-Process -Force
    Get-Process ffmpeg -ErrorAction SilentlyContinue | Stop-Process -Force
    Get-Process ffprobe -ErrorAction SilentlyContinue | Stop-Process -Force
}

cmake --preset $Preset
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

cmake --build --preset $Preset -j ([Environment]::ProcessorCount)
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($SmokeOnly) {
    ctest --test-dir $BuildDir --output-on-failure -R "TachyonSmokeTests"
} else {
    ctest --test-dir $BuildDir --output-on-failure
}

if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Cross-platform check OK" -ForegroundColor Green
