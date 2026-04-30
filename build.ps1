param(
    [ValidateSet('dev', 'dev-fast', 'asan')]
    [string]$Preset = 'dev',
    [switch]$RunTests,
    [switch]$ListTests,
    [string[]]$Target,
    [string]$TestFilter,
    [Nullable[UInt32]]$TestSeed,
    [Nullable[int]]$TestRepeat
)

$ErrorActionPreference = 'Stop'

function Get-DefaultTestFilter {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PresetName
    )

    switch ($PresetName) {
        'dev-fast' {
            return 'math,property,expression,frame_cache,frame_executor,tile_scheduler,render_contract,scene_evaluator,scene_spec,expression_vm'
        }
        default {
            return ''
        }
    }
}

function Assert-LastExitCode {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Step
    )

    if ($LASTEXITCODE -ne 0) {
        throw "$Step failed with exit code $LASTEXITCODE"
    }
}

function Invoke-TachyonTests {
    param(
        [Parameter(Mandatory = $true)]
        [string]$TestsExe,
        [string]$ResolvedTestFilter,
        [switch]$ListOnly,
        [Nullable[UInt32]]$Seed,
        [Nullable[int]]$Repeat
    )

    $hadTestFilter = Test-Path Env:TACHYON_TEST_FILTER
    $previousTestFilter = $env:TACHYON_TEST_FILTER
    $hadTestSeed = Test-Path Env:TACHYON_TEST_SEED
    $previousTestSeed = $env:TACHYON_TEST_SEED
    $hadTestRepeat = Test-Path Env:TACHYON_TEST_REPEAT
    $previousTestRepeat = $env:TACHYON_TEST_REPEAT

    if ($ResolvedTestFilter) {
        $env:TACHYON_TEST_FILTER = $ResolvedTestFilter
    }
    if ($null -ne $Seed) {
        $env:TACHYON_TEST_SEED = [string]$Seed
    }
    if ($null -ne $Repeat) {
        $env:TACHYON_TEST_REPEAT = [string]$Repeat
    }

    try {
        if ($ListOnly) {
            & $TestsExe --list-tests
        }
        else {
            & $TestsExe
        }
    }
    finally {
        if ($ResolvedTestFilter) {
            if ($hadTestFilter) {
                $env:TACHYON_TEST_FILTER = $previousTestFilter
            }
            else {
                Remove-Item Env:TACHYON_TEST_FILTER -ErrorAction SilentlyContinue
            }
        }

        if ($null -ne $Seed) {
            if ($hadTestSeed) {
                $env:TACHYON_TEST_SEED = $previousTestSeed
            }
            else {
                Remove-Item Env:TACHYON_TEST_SEED -ErrorAction SilentlyContinue
            }
        }

        if ($null -ne $Repeat) {
            if ($hadTestRepeat) {
                $env:TACHYON_TEST_REPEAT = $previousTestRepeat
            }
            else {
                Remove-Item Env:TACHYON_TEST_REPEAT -ErrorAction SilentlyContinue
            }
        }
    }
}

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$bootstrap = Join-Path $repoRoot 'scripts\Enable-DevTools.ps1'

if (-not (Test-Path $bootstrap)) {
    throw "Missing bootstrap script: $bootstrap"
}

& $bootstrap | Out-Null

switch ($Preset) {
    'asan' {
        $buildDir = Join-Path $repoRoot 'build\asan'
    }
    default {
        $buildDir = Join-Path $repoRoot 'build'
    }
}

$cacheFile = Join-Path $buildDir 'CMakeCache.txt'
$buildSystemFile = Join-Path $buildDir 'TACHYON.slnx'

if (-not (Test-Path $cacheFile) -or -not (Test-Path $buildSystemFile)) {
    cmake --preset $Preset
    Assert-LastExitCode -Step "cmake --preset $Preset"
}

$buildArgs = @('--build', '--preset', $Preset, '--parallel')
if ($Target -and $Target.Count -gt 0) {
    $buildArgs += '--target'
    $buildArgs += $Target
}

cmake @buildArgs
Assert-LastExitCode -Step "cmake --build --preset $Preset"

if ($RunTests -and $ListTests) {
    throw "Use either -RunTests or -ListTests, not both."
}

if ($ListTests) {
    $testsExe = Join-Path $buildDir 'tests\RelWithDebInfo\TachyonTests.exe'
    if (-not (Test-Path $testsExe)) {
        throw "Test binary not found: $testsExe"
    }

    $resolvedTestFilter = $TestFilter
    if (-not $resolvedTestFilter) {
        $resolvedTestFilter = Get-DefaultTestFilter -PresetName $Preset
    }

    Invoke-TachyonTests -TestsExe $testsExe -ResolvedTestFilter $resolvedTestFilter -ListOnly -Seed $TestSeed -Repeat $TestRepeat
}
elseif ($RunTests) {
    $testsExe = Join-Path $buildDir 'tests\RelWithDebInfo\TachyonTests.exe'
    if (-not (Test-Path $testsExe)) {
        throw "Test binary not found: $testsExe"
    }

    $resolvedTestFilter = $TestFilter
    if (-not $resolvedTestFilter) {
        $resolvedTestFilter = Get-DefaultTestFilter -PresetName $Preset
    }

    Invoke-TachyonTests -TestsExe $testsExe -ResolvedTestFilter $resolvedTestFilter -Seed $TestSeed -Repeat $TestRepeat
}
