<#
.SYNOPSIS
    Tachyon build entry point.
.PARAMETER Preset
    CMake preset: dev | dev-fast | asan  (default: dev)
.PARAMETER Config
    Debug | RelWithDebInfo | Release  (default: RelWithDebInfo)
.PARAMETER Target
    CMake/ninja target (default: from preset)
.PARAMETER Check
    Quick incremental build of TachyonCore via MSBuild. Skips CMake.
.PARAMETER Clean
    Clean artefacts before building
.PARAMETER Configure
    Force CMake reconfiguration
.PARAMETER Jobs
    Parallel jobs (default: CPU count)
.PARAMETER RunTests
    Run tests after building
.PARAMETER TestFilter
    Regex filter for test names (requires -RunTests)
.EXAMPLE
    .\build.ps1
    .\build.ps1 -Preset dev -RunTests
    .\build.ps1 -Preset dev-fast -RunTests -TestFilter frame_executor
    .\build.ps1 -Preset asan -RunTests -TestFilter math
    .\build.ps1 -Check
    .\build.ps1 -Target TachyonTests
    .\build.ps1 -Clean -Config Release
#>
param(
    [ValidateSet("dev","dev-fast","asan")]
    [string]$Preset   = "dev",
    [ValidateSet("Debug","RelWithDebInfo","Release")]
    [string]$Config   = "RelWithDebInfo",
    [string]$Target   = "",
    [switch]$Check,
    [switch]$Clean,
    [switch]$Configure,
    [int]$Jobs        = 0,
    [switch]$RunTests,
    [string]$TestFilter = "",
    [switch]$KillStaleTests
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$global:LASTEXITCODE = 0
$Root = $PSScriptRoot

# Auto-enable KillStaleTests on Windows
if (-not $KillStaleTests -and $env:OS -eq 'Windows_NT') {
    $KillStaleTests = $true
}

function Invoke-Native {
    param(
        [scriptblock]$Command,
        [string]$FailureMessage
    )

    $previousPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        & $Command
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousPreference
    }

    if ($exitCode -ne 0) {
        Write-Error $FailureMessage
        exit $exitCode
    }
}

if ($Check) {
    $Target = "TachyonCore"
}

# Determine build directory based on preset
if ($Preset -eq "asan") {
    $BuildDir = Join-Path $Root "build/asan"
} else {
    $BuildDir = Join-Path $Root "build"
}

Write-Host "Tachyon Build" -ForegroundColor Cyan
Write-Host "  Preset : $Preset"
Write-Host "  Config : $Config"
if ($Target) {
    Write-Host "  Target : $Target"
} else {
    Write-Host "  Target : (default from preset)"
}
if ($RunTests) {
    Write-Host "  Tests  : enabled" -ForegroundColor Yellow
}
if ($TestFilter) {
    Write-Host "  Filter : $TestFilter" -ForegroundColor Yellow
}

# Load tools into PATH
& "$Root\scripts\Enable-DevTools.ps1"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Load VS compiler environment (cached)
& "$Root\scripts\enable-vs-env.ps1"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Verify required tools exist
$missing = @()
foreach ($tool in @("cl","msbuild")) {
    if (-not (Get-Command "$tool.exe" -ErrorAction SilentlyContinue)) { $missing += $tool }
}
if ($missing.Count -gt 0) {
    Write-Error "Missing tools: $($missing -join ', '). Run scripts\enable-vs-env.ps1 to diagnose."
    exit 1
}

if ($KillStaleTests) {
    Write-Host "Killing stale Tachyon test processes..." -ForegroundColor Yellow
    Get-Process TachyonTests -ErrorAction SilentlyContinue | Stop-Process -Force
    Get-Process tachyon -ErrorAction SilentlyContinue | Stop-Process -Force
    Get-Process ffmpeg -ErrorAction SilentlyContinue | Stop-Process -Force
    Get-Process ffprobe -ErrorAction SilentlyContinue | Stop-Process -Force
}

if ($Clean) {
    Write-Host "Cleaning..." -ForegroundColor Yellow
    & "$Root\scripts\clean-all.ps1" -BuildOnly
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$NeedConf  = $Configure -or (-not (Test-Path (Join-Path $BuildDir "Tachyon.sln")))

if ($Check) {
    $CoreProject = Join-Path $BuildDir "src\TachyonCore.vcxproj"
    if (-not (Test-Path $CoreProject)) {
        Write-Error "Quick check requires an existing Visual Studio build tree. Run: .\build.ps1 -Preset dev"
        exit 1
    }

    Write-Host "Quick check (MSBuild, no CMake)..." -ForegroundColor Yellow
    Invoke-Native {
        & msbuild $CoreProject /m:1 /nologo "/p:Configuration=$Config" "/p:Platform=x64" "/t:Build"
    } "Quick check FAILED."
    Write-Host "Quick check OK" -ForegroundColor Green
    return
}

if ($NeedConf) {
    Write-Host "Configuring ($Preset)..." -ForegroundColor Yellow
    Invoke-Native { cmake --preset $Preset -S "$Root" -B "$BuildDir" } "CMake configure failed."
}

$J = if ($Jobs -gt 0) { $Jobs } else { [Environment]::ProcessorCount }

if ($Target) {
    Write-Host "Building $Target ($Config, $J jobs)..." -ForegroundColor Yellow
    Invoke-Native { cmake --build $BuildDir --config $Config --target $Target -j $J } "Build FAILED."
} else {
    Write-Host "Building preset targets: $Preset ($Config, $J jobs)..." -ForegroundColor Yellow
    Invoke-Native { cmake --build --preset $Preset --config $Config -j $J } "Build FAILED."
}

Write-Host "Build OK" -ForegroundColor Green

if ($RunTests) {
    Write-Host "Running tests..." -ForegroundColor Yellow

    if ($TestFilter) {
        Write-Host "  Internal filter: $TestFilter" -ForegroundColor Yellow

        $previousFilter = [Environment]::GetEnvironmentVariable("TACHYON_TEST_FILTER", "Process")
        [Environment]::SetEnvironmentVariable("TACHYON_TEST_FILTER", $TestFilter, "Process")

        try {
            Invoke-Native {
                & ctest --test-dir $BuildDir -C $Config --output-on-failure -R "^TachyonTests$"
            } "Tests FAILED."
        } finally {
            [Environment]::SetEnvironmentVariable("TACHYON_TEST_FILTER", $previousFilter, "Process")
        }
    } else {
        $ctestArgs = @("-C", $Config, "--output-on-failure", "-j", $J)
        Invoke-Native {
            & ctest --test-dir $BuildDir @ctestArgs
        } "Tests FAILED."
    }

    Write-Host "Tests OK" -ForegroundColor Green
}
