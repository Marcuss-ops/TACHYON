<#
.SYNOPSIS
    Tachyon build entry point.
.PARAMETER Config
    Debug | RelWithDebInfo | Release  (default: RelWithDebInfo)
.PARAMETER Target
    CMake/ninja target (default: tachyon)
.PARAMETER Check
    Quick validation only, no build
.PARAMETER Clean
    Clean artefacts before building
.PARAMETER Configure
    Force CMake reconfiguration
.PARAMETER Jobs
    Parallel jobs (default: CPU count)
.EXAMPLE
    .\build.ps1
    .\build.ps1 -Check
    .\build.ps1 -Target TachyonTests
    .\build.ps1 -Clean -Config Release
#>
param(
    [ValidateSet("Debug","RelWithDebInfo","Release")]
    [string]$Config   = "RelWithDebInfo",
    [string]$Target   = "tachyon",
    [switch]$Check,
    [switch]$Clean,
    [switch]$Configure,
    [int]$Jobs        = 0
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$global:LASTEXITCODE = 0
$Root = $PSScriptRoot

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

Write-Host "Tachyon Build" -ForegroundColor Cyan
Write-Host "  Config : $Config"
Write-Host "  Target : $Target"

# Load tools into PATH
& "$Root\scripts\Enable-DevTools.ps1"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Load VS compiler environment (cached)
& "$Root\scripts\enable-vs-env.ps1"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Verify required tools exist
$missing = @()
foreach ($tool in @("cmake","cl")) {
    if (-not (Get-Command "$tool.exe" -ErrorAction SilentlyContinue)) { $missing += $tool }
}
if ($missing.Count -gt 0) {
    Write-Error "Missing tools: $($missing -join ', '). Run scripts\Enable-DevTools.ps1 to diagnose."
    exit 1
}

if ($Clean) {
    Write-Host "Cleaning..." -ForegroundColor Yellow
    & "$Root\scripts\clean-all.ps1" -BuildOnly
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$BuildDir  = Join-Path $Root "build"
$NeedConf  = $Configure -or (-not (Test-Path (Join-Path $BuildDir "Tachyon.sln")))

if ($NeedConf) {
    Write-Host "Configuring..." -ForegroundColor Yellow
    Invoke-Native { cmake --preset dev -S "$Root" -B "$BuildDir" } "CMake configure failed."
}

$J = if ($Jobs -gt 0) { $Jobs } else { [Environment]::ProcessorCount }
Write-Host "Building $Target ($Config, $J jobs)..." -ForegroundColor Yellow

Invoke-Native { cmake --build $BuildDir --config $Config --target $Target -j $J } "Build FAILED."

Write-Host "Build OK" -ForegroundColor Green
