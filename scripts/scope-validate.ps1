# Scope Validator for AI Agents
# Verifies that an agent only modified files within its scope

param(
    [string]$Scope = "",
    [string]$BaseBranch = "main",
    [switch]$Strict
)

$ErrorActionPreference = 'Stop'

Write-Host "=== Scope Validator ===" -ForegroundColor Cyan

if (-not $Scope) {
    Write-Host "Usage: .\scripts\scope-validate.ps1 -Scope 'Component' [-BaseBranch 'main'] [-Strict]" -ForegroundColor Yellow
    Write-Host "Checks if modified files match the agent's scope." -ForegroundColor Yellow
    exit 1
}

# Get modified files
$modifiedFiles = git diff --name-only $BaseBranch...HEAD 2>$null
if (-not $modifiedFiles) {
    $modifiedFiles = git status --porcelain | ForEach-Object { $_.Substring(3) }
}

if (-not $modifiedFiles) {
    Write-Host "No modified files found." -ForegroundColor Yellow
    exit 0
}

Write-Host "`nModified files:" -ForegroundColor Yellow
$modifiedFiles | ForEach-Object { Write-Host "  $_" -ForegroundColor Cyan }

# Define scope-to-path mappings
$scopeMappings = @{
    "Component" = @("core/spec", "core/scene")
    "Scene" = @("core/scene", "core/spec/schema")
    "Renderer2D" = @("renderer2d")
    "Renderer3D" = @("renderer3d")
    "Text" = @("text", "renderer2d/text")
    "Audio" = @("audio", "renderer2d/audio")
    "Media" = @("media")
    "Timeline" = @("timeline")
    "Tracker" = @("tracker")
    "Runtime" = @("runtime")
    "Editor" = @("editor")
    "Build" = @("CMakeLists.txt", "build.ps1", "CMakePresets.json")
}

if (-not $scopeMappings.ContainsKey($Scope)) {
    Write-Host "`nWARNING: Unknown scope '$Scope'. Allowing all changes." -ForegroundColor Yellow
    Write-Host "Known scopes: $($scopeMappings.Keys -join ', ')" -ForegroundColor Yellow
    exit 0
}

# Check if files are within scope
$allowedPaths = $scopeMappings[$Scope]
$outOfScope = @()

foreach ($file in $modifiedFiles) {
    $isInScope = $false
    foreach ($allowedPath in $allowedPaths) {
        if ($file -match [regex]::Escape($allowedPath)) {
            $isInScope = $true
            break
        }
    }
    
    if (-not $isInScope) {
        $outOfScope += $file
    }
}

if ($outOfScope.Count -gt 0) {
    Write-Host "`nERROR: Files out of scope '$Scope':" -ForegroundColor Red
    foreach ($file in $outOfScope) {
        Write-Host "  $file" -ForegroundColor Red
    }
    
    if ($Strict) {
        Write-Host "`nScope validation FAILED (strict mode)" -ForegroundColor Red
        exit 1
    } else {
        Write-Host "`nWARNING: Out-of-scope files detected (use -Strict to fail)" -ForegroundColor Yellow
        exit 0
    }
} else {
    Write-Host "`nAll files are within scope '$Scope'" -ForegroundColor Green
    exit 0
}
