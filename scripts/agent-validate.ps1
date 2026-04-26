# scripts/agent-validate.ps1
# Usage: .\scripts\agent-validate.ps1 [-Mode quick|normal|full]
# TACHYON_VALIDATE_MODE env var overrides -Mode if set.

param(
    [string]$Mode = "",
    [switch]$Quick,
    [string]$Scope = ""
)

$ErrorActionPreference = 'Stop'

if ($Quick -and -not $Mode) { $Mode = 'quick' }
if (-not $Mode -and $env:TACHYON_VALIDATE_MODE) { $Mode = $env:TACHYON_VALIDATE_MODE }
if (-not $Mode) { $Mode = 'quick' }

$Mode = $Mode.ToLower()
if ($Mode -notin @('quick', 'normal', 'full')) {
    Write-Host "Unknown mode '$Mode'. Use: quick, normal, full" -ForegroundColor Red
    exit 1
}

Write-Host "=== Tachyon Validation [mode: $Mode] ===" -ForegroundColor Cyan

# -- shared: forbidden files ------------------------------------------------
Write-Host "`n[check] Forbidden files..." -ForegroundColor Yellow
$stagedFiles = git diff --cached --name-only 2>$null
$forbidden = @("*.obj", "*.pdb", "*.exe", "*.dll", "*.lib")
foreach ($file in $stagedFiles) {
    foreach ($pattern in $forbidden) {
        if ($file -like $pattern) {
            Write-Host "ERROR: staged forbidden file: $file" -ForegroundColor Red
            exit 1
        }
    }
}

# -- shared: no inline JSON in headers --------------------------------------
Write-Host "[check] Inline JSON in headers..." -ForegroundColor Yellow
$includeDir = Join-Path $PSScriptRoot "..\include\tachyon"
if (Test-Path $includeDir) {
    $headers = Get-ChildItem -Path $includeDir -Filter "*.h" -Recurse
    foreach ($header in $headers) {
        $content = Get-Content $header.FullName -Raw
        if ($content -match "void to_json\s*\([^)]*\)\s*\{" -or $content -match "void from_json\s*\([^)]*\)\s*\{") {
            Write-Host "WARNING: inline JSON in $($header.Name) -- move to .cpp" -ForegroundColor Yellow
        }
    }
}

# -- quick: syntax-only check on staged C++ files ---------------------------
$hasStagedCpp = ($stagedFiles | Where-Object { $_ -match '\.(cpp|h|hpp)$' }).Count -gt 0
$compileDb = Join-Path $PSScriptRoot "..\build-ninja\compile_commands.json"

if ($hasStagedCpp) {
    if (Test-Path $compileDb) {
        Write-Host "[check] Syntax check on staged C++ files..." -ForegroundColor Yellow
        & (Join-Path $PSScriptRoot "..\build.ps1") -Verify
        if ($LASTEXITCODE -ne 0) { Write-Host "Syntax check failed!" -ForegroundColor Red; exit 1 }
    } else {
        Write-Host "[skip] compile_commands.json not found -- run .\build.ps1 -Check once to generate it" -ForegroundColor Yellow
    }
}

if ($Mode -eq 'quick') {
    Write-Host "`n=== Validation PASSED [quick] ===" -ForegroundColor Green
    exit 0
}

# -- normal: incremental TachyonCore build ----------------------------------
Write-Host "`n[build] TachyonCore (incremental)..." -ForegroundColor Yellow
& (Join-Path $PSScriptRoot "..\build.ps1") -Check
if ($LASTEXITCODE -ne 0) { Write-Host "Core build failed!" -ForegroundColor Red; exit 1 }

if ($Mode -eq 'normal') {
    Write-Host "`n=== Validation PASSED [normal] ===" -ForegroundColor Green
    exit 0
}

# -- full: all tests --------------------------------------------------------
Write-Host "`n[build] All tests..." -ForegroundColor Yellow
& (Join-Path $PSScriptRoot "..\build.ps1") -RelWithDebInfo -TestsOnly
if ($LASTEXITCODE -ne 0) { Write-Host "Test build failed!" -ForegroundColor Red; exit 1 }

Write-Host "`n[run] Tests..." -ForegroundColor Yellow
& (Join-Path $PSScriptRoot "..\build.ps1") -RelWithDebInfo -Test
if ($LASTEXITCODE -ne 0) { Write-Host "Tests failed!" -ForegroundColor Red; exit 1 }

Write-Host "`n=== Validation PASSED [full] ===" -ForegroundColor Green
exit 0
