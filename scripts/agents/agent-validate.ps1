# scripts/agent-validate.ps1
# One-command validation for agents and contributors.
# It keeps output short on success and shows only the most relevant
# failure lines when something breaks.

param(
    [string]$Mode = "",
    [switch]$Quick,
    [string]$Scope = ""
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot

if ($Quick -and -not $Mode) { $Mode = 'quick' }
if (-not $Mode -and $env:TACHYON_VALIDATE_MODE) { $Mode = $env:TACHYON_VALIDATE_MODE }
if (-not $Mode) { $Mode = 'quick' }

$Mode = $Mode.ToLowerInvariant()
if ($Mode -notin @('quick', 'normal', 'full')) {
    Write-Host "Unknown mode '$Mode'. Use: quick, normal, full" -ForegroundColor Red
    exit 1
}

function Get-InterestingLogLines {
    param([string[]]$Lines)

    $patterns = @(
        '(^|:) error C\d+',
        'CMake Error',
        'fatal error',
        'Write-Error:',
        'FAILED',
        'Build FAILED',
        'cannot find',
        'cannot open',
        'not a member',
        'is not a member',
        'is not declared',
        'non è possibile',
        'non è un membro'
    )

    $matched = @()
    foreach ($pattern in $patterns) {
        $matched += $Lines | Select-String -Pattern $pattern -SimpleMatch:$false
    }

    $unique = @{}
    foreach ($m in $matched) {
        $line = $m.Line.Trim()
        if ($line -and -not $unique.ContainsKey($line)) {
            $unique[$line] = $true
        }
    }

    return $unique.Keys
}

function Invoke-Step {
    param(
        [string]$Name,
        [scriptblock]$Command
    )

    $stamp = [Guid]::NewGuid().ToString('N')
    $logPath = Join-Path $env:TEMP "tachyon-validate-$stamp.log"
    $exitCode = 0

    Write-Host "[step] $Name" -ForegroundColor Yellow

    try {
        & $Command *>&1 | Tee-Object -FilePath $logPath | Out-Null
        $exitCode = $LASTEXITCODE
        if ($exitCode -eq 0) {
            Write-Host "  OK" -ForegroundColor Green
            return $true
        }
    } catch {
        $exitCode = if ($LASTEXITCODE) { $LASTEXITCODE } else { 1 }
    } finally {
    }

    $lines = @()
    if (Test-Path $logPath) {
        $lines = Get-Content $logPath
    }
    $interesting = @(Get-InterestingLogLines -Lines $lines)
    Write-Host "  FAIL (exit $exitCode)" -ForegroundColor Red
    if ($interesting.Count -gt 0) {
        Write-Host "  First useful lines:" -ForegroundColor Yellow
        $interesting | Select-Object -First 12 | ForEach-Object {
            Write-Host "    $_" -ForegroundColor Red
        }
    } else {
        Write-Host "  No filtered error lines found. Showing the first 12 log lines:" -ForegroundColor Yellow
        $lines | Select-Object -First 12 | ForEach-Object {
            Write-Host "    $_" -ForegroundColor Red
        }
    }
    if (Test-Path $logPath) {
        Remove-Item $logPath -Force -ErrorAction SilentlyContinue
    }
    return $false
}

Write-Host "=== Tachyon validation [$Mode] ===" -ForegroundColor Cyan

if ($Scope) {
    if (-not (Invoke-Step "Scope check [$Scope]" { & (Join-Path $Root "scripts\scope-validate.ps1") -Scope $Scope -Strict })) {
        exit 1
    }
}

if (-not (Invoke-Step "Core build" { & (Join-Path $Root "build.ps1") -Check })) {
    exit 1
}

if ($Mode -eq 'quick') {
    Write-Host "=== Validation PASSED [quick] ===" -ForegroundColor Green
    exit 0
}

if (-not (Invoke-Step "Tests build" { & (Join-Path $Root "build.ps1") -Target TachyonTests -Config RelWithDebInfo })) {
    exit 1
}

if ($Mode -eq 'normal') {
    Write-Host "=== Validation PASSED [normal] ===" -ForegroundColor Green
    exit 0
}

if (-not (Invoke-Step "Run tests" {
    & (Join-Path $Root "build\tests\RelWithDebInfo\TachyonTests.exe")
})) {
    exit 1
}

Write-Host "=== Validation PASSED [full] ===" -ForegroundColor Green
exit 0
