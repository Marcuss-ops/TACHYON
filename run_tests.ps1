param(
    [string]$Filter = "",
    [switch]$Verbose,
    [switch]$NoBuild
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $repoRoot 'build-ninja'
$testExe = Join-Path $buildDir 'tests' 'TachyonTests.exe'

if (-not $NoBuild) {
    Write-Host "Building tests..." -ForegroundColor Cyan
    & "$repoRoot\build.ps1" -Targets @('TachyonTests') -Fast
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed with exit code $LASTEXITCODE"
    }
}

if (-not (Test-Path -LiteralPath $testExe)) {
    throw "Test executable not found: $testExe"
}

$srcDir = Join-Path $buildDir 'src'
$testsDir = Join-Path $buildDir 'tests'

$env:PATH = "$srcDir;$testsDir;" + $env:PATH

Write-Host "Test PATH includes: $srcDir, $testsDir" -ForegroundColor Cyan
Write-Host "Running tests..." -ForegroundColor Cyan

$testArgs = @()
if ($Filter) {
    $testArgs += "--gtest_filter=$Filter"
}
if ($Verbose) {
    $testArgs += '--gtest_verbose=1'
}

& $testExe $testArgs
$exitCode = $LASTEXITCODE

if ($exitCode -eq 0) {
    Write-Host "All tests passed!" -ForegroundColor Green
} else {
    Write-Host "Tests failed with exit code $exitCode" -ForegroundColor Red
}

exit $exitCode
