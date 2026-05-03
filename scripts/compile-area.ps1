# Scope-aware build helper for Tachyon.
# Picks the smallest useful build target for the touched area.

param(
    [ValidateSet("auto","headers","core","tests","full")]
    [string]$Area = "auto",
    [string[]]$Paths = @(),
    [switch]$RunTests,
    [string]$TestFilter = "",
    [switch]$Explain
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot

function Get-InputPaths {
    param([string[]]$ExplicitPaths)

    if ($ExplicitPaths -and $ExplicitPaths.Count -gt 0) {
        return $ExplicitPaths
    }

    $status = git status --porcelain 2>$null
    if (-not $status) {
        return @()
    }

    return $status | ForEach-Object { $_.Substring(3) }
}

function Get-DetectedArea {
    param([string[]]$InputPaths)

    if (-not $InputPaths -or $InputPaths.Count -eq 0) {
        return "core"
    }

    $hasTests = $false
    $hasSource = $false
    $hasHeaders = $false
    $hasBuildFiles = $false

    foreach ($path in $InputPaths) {
        if ($path -match '(^|[\\/])tests([\\/]|$)' -or $path -match '_tests?\.(cpp|cxx|cc|h|hpp)$' -or $path -match '^test_') {
            $hasTests = $true
            continue
        }

        if ($path -match '\.(h|hpp|inl)$' -and $path -match '(^|[\\/])include([\\/]|$)') {
            $hasHeaders = $true
            continue
        }

        if ($path -match '\.(cpp|cxx|cc)$' -and $path -match '(^|[\\/])src([\\/]|$)') {
            $hasSource = $true
            continue
        }

        if ($path -match '(^|[\\/])(CMakeLists\.txt|CMakePresets\.json|build\.ps1|build\.cmd|scripts[\\/])') {
            $hasBuildFiles = $true
        }
    }

    if ($hasTests -and -not $hasSource -and -not $hasHeaders) {
        return "tests"
    }

    if ($hasHeaders -and -not $hasSource -and -not $hasTests -and -not $hasBuildFiles) {
        return "headers"
    }

    if ($hasBuildFiles) {
        return "core"
    }

    return "core"
}

function Invoke-SelectedBuild {
    param([string]$SelectedArea)

    switch ($SelectedArea) {
        "headers" {
            Write-Host "[build] HeaderSmokeTests (smallest header-only check)" -ForegroundColor Yellow
            & "$Root\build.ps1" -Target HeaderSmokeTests
            return $LASTEXITCODE
        }
        "tests" {
            Write-Host "[build] TachyonTests (targeted test build)" -ForegroundColor Yellow
            if ($RunTests -and $TestFilter) {
                & "$Root\build.ps1" -Preset dev-fast -RunTests -TestFilter $TestFilter
            } elseif ($RunTests) {
                & "$Root\build.ps1" -Preset dev-fast -RunTests
            } else {
                & "$Root\build.ps1" -Target TachyonTests -Config RelWithDebInfo
            }
            return $LASTEXITCODE
        }
        "full" {
            Write-Host "[build] Full dev preset" -ForegroundColor Yellow
            & "$Root\build.ps1" -Preset dev -RunTests
            return $LASTEXITCODE
        }
        default {
            Write-Host "[build] TachyonCore quick check" -ForegroundColor Yellow
            if ($RunTests -and $TestFilter) {
                & "$Root\build.ps1" -Preset dev-fast -RunTests -TestFilter $TestFilter
            } elseif ($RunTests) {
                & "$Root\build.ps1" -Preset dev-fast -RunTests
            } else {
                & "$Root\build.ps1" -Check
            }
            return $LASTEXITCODE
        }
    }
}

$inputPaths = Get-InputPaths -ExplicitPaths $Paths
$selectedArea = $Area
if ($selectedArea -eq "auto") {
    $selectedArea = Get-DetectedArea -InputPaths $inputPaths
}

if ($Explain) {
    Write-Host "=== Compile Area Planner ===" -ForegroundColor Cyan
    if ($inputPaths.Count -gt 0) {
        Write-Host "Input files:" -ForegroundColor Yellow
        $inputPaths | ForEach-Object { Write-Host "  $_" -ForegroundColor Cyan }
    } else {
        Write-Host "Input files: (git status or explicit paths were empty)" -ForegroundColor Yellow
    }
    Write-Host "Selected area: $selectedArea" -ForegroundColor Yellow
}

$exitCode = Invoke-SelectedBuild -SelectedArea $selectedArea
exit $exitCode
