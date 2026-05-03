# PowerShell pre-commit hook installer for Tachyon
# Usage: .\scripts\pre-commit.ps1 -Install

param([switch]$Install)

$ErrorActionPreference = 'Stop'
$hookDir = ".git/hooks"
$scriptPath = (Resolve-Path "$PSScriptRoot\agent-validate.ps1").Path

if ($Install) {
    Write-Host "Installing git hooks..." -ForegroundColor Cyan

    if (-not (Test-Path $hookDir)) {
        Write-Host "Not a git repository!" -ForegroundColor Red
        exit 1
    }

    $preCommitContent = "#!/bin/sh`n# Tachyon pre-commit hook`n# Override: TACHYON_VALIDATE_MODE=normal git commit`nMODE=`"`${TACHYON_VALIDATE_MODE:-quick}`"`nexec powershell -ExecutionPolicy Bypass -File `"$scriptPath`" -Mode `"`$MODE`"`n"
    Set-Content -Path "$hookDir/pre-commit" -Value $preCommitContent -NoNewline

    $prePushContent = "#!/bin/sh`n# Tachyon pre-push hook -- fast compile-only validation`nexec powershell -ExecutionPolicy Bypass -File `"$scriptPath`" -Mode normal`n"
    Set-Content -Path "$hookDir/pre-push" -Value $prePushContent -NoNewline

    Write-Host "pre-commit installed (default: quick ~15s)" -ForegroundColor Green
    Write-Host "pre-push   installed (default: normal ~45s)" -ForegroundColor Green
    Write-Host ""
    Write-Host "Usage:" -ForegroundColor Cyan
    Write-Host "  git commit                               -> quick (~15s)"
    Write-Host "  TACHYON_VALIDATE_MODE=normal git commit  -> normal (~45s)"
    Write-Host "  TACHYON_VALIDATE_MODE=full git commit    -> full (~3-5min)"
    Write-Host "  git push                                 -> normal (~45s)"
    exit 0
}

Write-Host "Run with -Install to install pre-commit and pre-push hooks." -ForegroundColor Yellow
Write-Host "Or run .\scripts\agent-validate.ps1 -Mode quick|normal|full directly."
