# Quick validation script for AI agents
# Usage: .\scripts\agent-validate.ps1 -Scope "Component"

param(
    [string]$Scope = "",
    [switch]$Quick
)

$ErrorActionPreference = 'Stop'

Write-Host "=== Tachyon Agent Validation ===" -ForegroundColor Cyan

# Step 1: Header smoke tests
Write-Host "`n[1/4] Running header smoke tests..." -ForegroundColor Yellow
Write-Host "DEBUG CWD: $(Get-Location)" -ForegroundColor Magenta
Write-Host "DEBUG build.ps1 exists: $(Test-Path '.\build.ps1')" -ForegroundColor Magenta
try { & .\build.ps1 -RelWithDebInfo -TestsOnly } catch { Write-Host "DEBUG: threw: $_" -ForegroundColor Magenta }
Write-Host "DEBUG: LASTEXITCODE=$LASTEXITCODE  ?=$?" -ForegroundColor Magenta
if ($LASTEXITCODE -ne 0) {
    Write-Host "Header smoke tests failed!" -ForegroundColor Red
    exit 1
}

# Step 2: Core build check
Write-Host "`n[2/4] Building TachyonCore..." -ForegroundColor Yellow
& .\build.ps1 -Check
if ($LASTEXITCODE -ne 0) {
    Write-Host "Core build failed!" -ForegroundColor Red
    exit 1
}

# Step 3: Targeted tests if scope provided
if ($Scope -and -not $Quick) {
    Write-Host "`n[3/4] Running targeted tests for: $Scope" -ForegroundColor Yellow
    & .\build.ps1 -RelWithDebInfo -TestFilter $Scope
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Targeted tests failed!" -ForegroundColor Red
        exit 1
    }
}

# Step 4: Full build (only if not quick mode)
if (-not $Quick) {
    Write-Host "`n[4/4] Running full build..." -ForegroundColor Yellow
    & .\build.ps1 -RelWithDebInfo
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Full build failed!" -ForegroundColor Red
        exit 1
    }
}

Write-Host "`n=== Validation PASSED ===" -ForegroundColor Green
