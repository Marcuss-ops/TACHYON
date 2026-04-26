# Development Environment Setup Script for Tachyon
# Run this after cloning the repository

param(
    [switch]$InstallTools
)

$ErrorActionPreference = 'Stop'

Write-Host "=== Tachyon Development Setup ===" -ForegroundColor Cyan

# Step 1: Generate compile_commands.json
Write-Host "`n[1/4] Generating compile_commands.json..." -ForegroundColor Yellow
& .\build.ps1 -Check -Reconfigure
if ($LASTEXITCODE -ne 0) {
    Write-Host "Failed to generate compile_commands.json!" -ForegroundColor Red
    exit 1
}

# Step 2: Verify clangd configuration
Write-Host "`n[2/4] Verifying clangd configuration..." -ForegroundColor Yellow
if (Test-Path ".clangd") {
    Write-Host "  .clangd found" -ForegroundColor Green
} else {
    Write-Host "  .clangd missing!" -ForegroundColor Red
}

if (Test-Path "build-ninja/compile_commands.json") {
    Write-Host "  compile_commands.json found" -ForegroundColor Green
} else {
    Write-Host "  compile_commands.json missing!" -ForegroundColor Red
}

# Step 3: Install tools (optional)
if ($InstallTools) {
    Write-Host "`n[3/4] Installing development tools..." -ForegroundColor Yellow
    
    # Check if winget is available
    if (Get-Command winget -ErrorAction SilentlyContinue) {
        Write-Host "  Installing LLVM (clangd)..." -ForegroundColor Cyan
        winget install LLVM.LLVM -e --silent
    } else {
        Write-Host "  winget not found. Please install LLVM manually from https://llvm.org/" -ForegroundColor Yellow
    }
}

# Step 4: Create necessary directories
Write-Host "`n[4/4] Creating project directories..." -ForegroundColor Yellow
$dirs = @("tests/compile", ".vscode", "scripts")
foreach ($dir in $dirs) {
    if (-not (Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
        Write-Host "  Created: $dir" -ForegroundColor Green
    }
}

Write-Host "`n=== Setup Complete ===" -ForegroundColor Green
Write-Host "`nNext steps:" -ForegroundColor Cyan
Write-Host "  1. Open VS Code and install clangd extension"
Write-Host "  2. Press Ctrl+Shift+B to access build tasks"
Write-Host "  3. Run .\build.ps1 -Check for quick builds"
Write-Host "  4. Read AGENTS.md for AI agent guidelines"
