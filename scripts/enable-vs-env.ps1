# Enable Visual Studio Environment for Current Session
# Run this once in your PowerShell session to use ninja/cmake/cl.exe directly

param(
    [string]$VSVersion = "2022",
    [string]$VSEdition = "Community",
    [switch]$PersistUserPath
)

$ErrorActionPreference = 'Stop'

Write-Host "=== Enabling Visual Studio Environment ===" -ForegroundColor Cyan

# Find vcvars64.bat
$possiblePaths = @(
    "C:\Program Files\Microsoft Visual Studio\$VSVersion\Community\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\$VSVersion\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\17\Community\VC\Auxiliary\Build\vcvars64.bat"
)

$vcvars = $null
foreach ($path in $possiblePaths) {
    if (Test-Path $path) {
        $vcvars = $path
        break
    }
}

if (-not $vcvars) {
    Write-Host "ERROR: vcvars64.bat not found!" -ForegroundColor Red
    Write-Host "Searched:" -ForegroundColor Yellow
    foreach ($path in $possiblePaths) {
        Write-Host "  $path" -ForegroundColor Yellow
    }
    exit 1
}

Write-Host "Found: $vcvars" -ForegroundColor Green

# Create a batch file to dump environment
$tempBat = [System.IO.Path]::GetTempFileName() + ".bat"
$batContent = "@echo off`r`n"
$batContent += "call `"$vcvars`" >nul 2>&1`r`n"
$batContent += "echo __ENV_START__`r`n"
$batContent += "set`r`n"
$batContent += "echo __ENV_END__`r`n"
Set-Content -Path $tempBat -Value $batContent -Encoding ASCII

Write-Host "`nLoading Visual Studio environment..." -ForegroundColor Yellow

$envOutput = cmd /c $tempBat 2>&1
Remove-Item $tempBat -Force

# Parse environment variables
$capture = $false
$envVars = @{}
foreach ($line in $envOutput) {
    if ($line -eq "__ENV_START__") { $capture = $true; continue }
    if ($line -eq "__ENV_END__") { $capture = $false; continue }
    if ($capture -and $line -match "^(.+?)=(.+)$") {
        $envVars[$Matches[1]] = $Matches[2]
    }
}

# Apply critical environment variables
$criticalVars = @("PATH", "INCLUDE", "LIB", "LIBPATH", "VCINSTALLDIR", "VSINSTALLDIR")
foreach ($var in $criticalVars) {
    if ($envVars.ContainsKey($var)) {
        Set-Item -Path "env:$var" -Value $envVars[$var]
        Write-Host "  Set $var" -ForegroundColor Green
    }
}

# Verify tools are now accessible
Write-Host "`nVerifying tools..." -ForegroundColor Yellow
$tools = @("cl.exe", "ninja.exe", "cmake.exe")
foreach ($tool in $tools) {
    $cmd = Get-Command $tool -ErrorAction SilentlyContinue
    if ($cmd) {
        Write-Host "  [OK] $tool found: $($cmd.Source)" -ForegroundColor Green
    } else {
        Write-Host "  [FAIL] $tool NOT found in PATH" -ForegroundColor Red
    }
}

Write-Host "`n=== Environment Ready ===" -ForegroundColor Green
Write-Host "You can now run these commands directly:" -ForegroundColor Cyan
Write-Host "  ninja -C build-ninja TachyonCore" -ForegroundColor Cyan
Write-Host "  cmake --build build-ninja --preset relwithdebinfo" -ForegroundColor Cyan
Write-Host "  cl.exe /Zs file.cpp (syntax check)" -ForegroundColor Cyan

if ($PersistUserPath) {
    Write-Host "`nPersisting to user PATH..." -ForegroundColor Yellow
    $currentPath = [Environment]::GetEnvironmentVariable("PATH", "User")
    if ($currentPath -notlike "*$env:USERPROFILE*") {
        $newPath = $currentPath + ";" + $env:PATH
        [Environment]::SetEnvironmentVariable("PATH", $newPath, "User")
        Write-Host "PATH persisted. Restart PowerShell to use new PATH." -ForegroundColor Green
    } else {
        Write-Host "PATH already persisted." -ForegroundColor Yellow
    }
}
