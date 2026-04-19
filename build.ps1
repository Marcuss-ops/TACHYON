param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [switch]$RunTests
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$bootstrap = Join-Path $repoRoot 'scripts\Enable-DevTools.ps1'

if (-not (Test-Path $bootstrap)) {
    throw "Missing bootstrap script: $bootstrap"
}

& $bootstrap | Out-Null

$buildDir = Join-Path $repoRoot 'build'
if (-not (Test-Path (Join-Path $buildDir 'CMakeCache.txt'))) {
    cmake -S $repoRoot -B $buildDir
}

cmake --build $buildDir --config $Configuration

if ($RunTests) {
    $testsExe = Join-Path $buildDir "tests\$Configuration\TachyonTests.exe"
    if (-not (Test-Path $testsExe)) {
        throw "Test binary not found: $testsExe"
    }

    & $testsExe
}
