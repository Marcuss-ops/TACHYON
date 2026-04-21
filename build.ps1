param(
    [ValidateSet('dev', 'dev-fast', 'asan')]
    [string]$Preset = 'dev',
    [switch]$RunTests,
    [string[]]$Target,
    [string]$TestFilter
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

if ($RunTests) {
    $testsExe = Join-Path $buildDir 'tests\RelWithDebInfo\TachyonTests.exe'
    if (-not (Test-Path $testsExe)) {
        throw "Test binary not found: $testsExe"
    }

    $resolvedTestFilter = $TestFilter
    if (-not $resolvedTestFilter) {
        $resolvedTestFilter = Get-DefaultTestFilter -PresetName $Preset
    }

    $hadTestFilter = Test-Path Env:TACHYON_TEST_FILTER
    $previousTestFilter = $env:TACHYON_TEST_FILTER

    if ($resolvedTestFilter) {
        $env:TACHYON_TEST_FILTER = $resolvedTestFilter
    }

    try {
        & $testsExe
    }
    finally {
        if ($resolvedTestFilter) {
            if ($hadTestFilter) {
                $env:TACHYON_TEST_FILTER = $previousTestFilter
            }
            else {
                Remove-Item Env:TACHYON_TEST_FILTER -ErrorAction SilentlyContinue
            }
        }
    }
}
