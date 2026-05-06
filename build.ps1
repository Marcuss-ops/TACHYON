<#
.SYNOPSIS
    Tachyon build entry point.
.PARAMETER Preset
    CMake preset: dev | dev-fast | asan | linux-ci | windows-ci | release  (default: dev on Windows, linux-ci on Linux)
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
    [ValidateSet("dev","dev-fast","asan","linux-ci","windows-ci","release")]
    [string]$Preset   = "",
    [ValidateSet("Debug","RelWithDebInfo","Release")]
    [string]$Config   = "RelWithDebInfo",
    [string]$Target   = "",
    [switch]$Check,
    [switch]$Clean,
    [switch]$Configure,
    [alias("Parallel")]
    [int]$Jobs        = 0,
    [switch]$RunTests,
    [string]$TestFilter = "",
    [switch]$KillStaleTests,
    [switch]$SkipBuild
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

function Prepend-ProcessPath {
    param(
        [string[]]$Paths
    )

    $current = [Environment]::GetEnvironmentVariable("PATH", "Process")
    $separator = [IO.Path]::PathSeparator
    $existing = @()
    foreach ($part in $current -split [Regex]::Escape($separator)) {
        if ($part -and -not $existing.Contains($part)) {
            $existing += $part
        }
    }

    $prefix = @()
    foreach ($path in $Paths) {
        if ($path -and (Test-Path $path) -and -not $prefix.Contains($path) -and -not $existing.Contains($path)) {
            $prefix += $path
        }
    }

    if ($prefix.Count -gt 0) {
        $newPath = ($prefix + $existing) -join $separator
        [Environment]::SetEnvironmentVariable("PATH", $newPath, "Process")
    }
}

function Get-TestTargetForFilter {
    param(
        [string]$Filter
    )

    $normalized = ($Filter.ToLowerInvariant() -replace '[^a-z0-9]', '')
    if ($normalized -match 'nativerender') {
        return "TachyonNativeRenderTests"
    }

    if ($normalized -match 'rendersession|png3dvalidation|motionblur|frameblend|timeremap|rollingshutter|threedmodifier|parallaxcards') {
        return "TachyonRenderPipelineTests"
    }

    if ($normalized -match 'renderprofiler') {
        return "TachyonRenderProfilerTests"
    }

    if ($normalized -match 'scene3dsmoke') {
        return "TachyonScene3DSmokeTests"
    }

    if ($normalized -match 'assetresolution|imagemanager|imagedecode|framebuffer|rasterizer|surface|drawlistbuilder|blendmodes|evaluatedcompositionrenderer|pathrasterizer|effecthost|matteresolver') {
        return "TachyonRenderTests"
    }

    if ($normalized -match 'studio|glyphcache|text|audiopitch|audiotrim|backgroundpreset|transition|sfxcontract|audit') {
        return "TachyonContentTests"
    }

    if ($normalized -match 'scene_|timeline|camera_|motion_map|default_camera') {
        return "TachyonSceneTests"
    }

    return "TachyonTests"
}

if ($Check) {
    $Target = "TachyonCore"
}

if ($RunTests -and $TestFilter -and -not $Target) {
    $Target = Get-TestTargetForFilter $TestFilter
}

if (-not $Preset) {
    if ($env:OS -eq 'Windows_NT') {
        $Preset = "dev"
    } else {
        $Preset = "linux-ci"
    }
}

function Test-IsWindowsPreset {
    param([string]$Name)
    return $Name -in @("dev", "dev-fast", "asan")
}

# Determine build directory based on preset
$BuildDir = switch ($Preset) {
    "asan" { Join-Path $Root "build/asan" }
    "linux-ci" { Join-Path $Root "build/linux-ci" }
    "windows-ci" { Join-Path $Root "build/windows-ci" }
    "release" { Join-Path $Root "build/release" }
    default { Join-Path $Root "build" }
}
$J = if ($Jobs -gt 0) { $Jobs } else { [Environment]::ProcessorCount }

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
if ($SkipBuild) {
    Write-Host "  Build  : SKIPPED" -ForegroundColor Yellow
}

if ($env:OS -eq 'Windows_NT') {
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
} else {
    $missing = @()
    foreach ($tool in @("cmake","ctest")) {
        if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) { $missing += $tool }
    }
    if ($missing.Count -gt 0) {
        Write-Error "Missing tools: $($missing -join ', '). Install cmake and ctest."
        exit 1
    }
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
    if ($env:OS -eq 'Windows_NT' -and (Test-IsWindowsPreset $Preset)) {
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

    Write-Host "Quick check (build preset, no reconfigure)..." -ForegroundColor Yellow
    Invoke-Native {
        & cmake --build --preset $Preset --target $Target -j $J
    } "Quick check FAILED."
    Write-Host "Quick check OK" -ForegroundColor Green
    return
}

if ($NeedConf) {
    Write-Host "Configuring ($Preset)..." -ForegroundColor Yellow
    Invoke-Native { cmake --preset $Preset -S "$Root" -B "$BuildDir" } "CMake configure failed."
}

if (-not $SkipBuild) {
    if ($env:OS -eq 'Windows_NT' -and (Test-IsWindowsPreset $Preset)) {
        if ($Target) {
            Write-Host "Building $Target ($Config, $J jobs)..." -ForegroundColor Yellow
            Invoke-Native { cmake --build $BuildDir --config $Config --target $Target -j $J } "Build FAILED."
        } else {
            Write-Host "Building preset targets: $Preset ($Config, $J jobs)..." -ForegroundColor Yellow
            Invoke-Native { cmake --build --preset $Preset --config $Config -j $J } "Build FAILED."
        }
    } else {
        if ($Target) {
            Write-Host "Building $Target ($J jobs)..." -ForegroundColor Yellow
            Invoke-Native { cmake --build --preset $Preset --target $Target -j $J } "Build FAILED."
        } else {
            Write-Host "Building preset targets: $Preset ($J jobs)..." -ForegroundColor Yellow
            Invoke-Native { cmake --build --preset $Preset -j $J } "Build FAILED."
        }
    }
}

Write-Host "Build OK" -ForegroundColor Green

if ($RunTests) {
    Write-Host "Running tests..." -ForegroundColor Yellow

    $binDirs = if ($env:OS -eq 'Windows_NT' -and (Test-IsWindowsPreset $Preset)) {
        @(
            (Join-Path $BuildDir "src\$Config"),
            (Join-Path $BuildDir "tests\$Config")
        )
    } else {
        @(
            (Join-Path $BuildDir "src"),
            (Join-Path $BuildDir "tests")
        )
    }
    Prepend-ProcessPath $binDirs

    $selectedTestTarget = if ($TestFilter) { Get-TestTargetForFilter $TestFilter } else { "" }
    # Build already handled by the main build block if -SkipBuild was not set
    # and $Target was auto-assigned from filter.

    if ($TestFilter) {
        Write-Host "  Internal filter: $TestFilter" -ForegroundColor Yellow
        $testTarget = if ($selectedTestTarget) { $selectedTestTarget } else { Get-TestTargetForFilter $TestFilter }
        Write-Host "  Test target: $testTarget" -ForegroundColor Yellow

        $previousFilter = [Environment]::GetEnvironmentVariable("TACHYON_TEST_FILTER", "Process")
        [Environment]::SetEnvironmentVariable("TACHYON_TEST_FILTER", $TestFilter, "Process")

        try {
            Invoke-Native {
                if ($env:OS -eq 'Windows_NT' -and (Test-IsWindowsPreset $Preset)) {
                    & ctest --test-dir $BuildDir -C $Config --output-on-failure -R "^$testTarget$"
                } else {
                    & ctest --test-dir $BuildDir --output-on-failure -R "^$testTarget$"
                }
            } "Tests FAILED."
        } finally {
            [Environment]::SetEnvironmentVariable("TACHYON_TEST_FILTER", $previousFilter, "Process")
        }
    } else {
        $ctestArgs = if ($env:OS -eq 'Windows_NT' -and (Test-IsWindowsPreset $Preset)) {
            @("-C", $Config, "--output-on-failure", "-j", $J)
        } else {
            @("--output-on-failure", "-j", $J)
        }
        Invoke-Native {
            & ctest --test-dir $BuildDir @ctestArgs
        } "Tests FAILED."
    }

    Write-Host "Tests OK" -ForegroundColor Green
}
