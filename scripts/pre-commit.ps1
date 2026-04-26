# PowerShell pre-commit hook for Tachyon
# Auto-installs and runs checks before commits

param([switch]$Install)

$ErrorActionPreference = 'Stop'
$hookDir = ".git/hooks"
$hookPath = "$hookDir/pre-commit"
$scriptPath = "$PSScriptRoot/agent-validate.ps1"

if ($Install) {
    Write-Host "Installing pre-commit hook..." -ForegroundColor Cyan
    
    if (-not (Test-Path $hookDir)) {
        Write-Host "Not a git repository!" -ForegroundColor Red
        exit 1
    }
    
    # Create PowerShell pre-commit hook
    $hookContent = @"
# Tachyon pre-commit hook (PowerShell)
# Runs quick validation before allowing commit

`$ErrorActionPreference = 'Stop'

`$scriptPath = '$scriptPath'
if (Test-Path `$scriptPath) {
    powershell -ExecutionPolicy Bypass -File "`$scriptPath" -Quick
    if (`$LASTEXITCODE -ne 0) {
        Write-Host "Pre-commit checks failed! Fix errors before committing." -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "Warning: agent-validate.ps1 not found" -ForegroundColor Yellow
}

exit 0
"@
    
    Set-Content -Path $hookPath -Value $hookContent
    Write-Host "Pre-commit hook installed at $hookPath" -ForegroundColor Green
    exit 0
}

# If not installing, run the validation
Write-Host "=== Pre-commit Check ===" -ForegroundColor Cyan

# Step 1: Check for forbidden files
Write-Host "`n[1/4] Checking for forbidden files..." -ForegroundColor Yellow
$forbidden = @("*.obj", "*.pdb", "*.exe", "*.dll")
$files = git diff --cached --name-only 2>$null
foreach ($file in $files) {
    foreach ($pattern in $forbidden) {
        if ($file -like $pattern) {
            Write-Host "ERROR: Attempting to commit forbidden file: $file" -ForegroundColor Red
            exit 1
        }
    }
}

# Step 2: Quick build check
Write-Host "`n[2/4] Running quick build check..." -ForegroundColor Yellow
& .\build.ps1 -Check -ErrorsOnly 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build check failed! Fix errors before committing." -ForegroundColor Red
    exit 1
}

# Step 3: Header smoke tests
Write-Host "`n[3/4] Running header smoke tests..." -ForegroundColor Yellow
$output = cmake --build build-ninja --preset relwithdebinfo --target HeaderSmokeTests 2>&1
$errors = $output | Select-String -Pattern "FAILED|error C"
if ($errors) {
    Write-Host "Header smoke tests failed!" -ForegroundColor Red
    $errors
    exit 1
}

# Step 4: Check for inline JSON in headers
Write-Host "`n[4/4] Checking for inline JSON serialization in headers..." -ForegroundColor Yellow
$headers = Get-ChildItem -Path "include/tachyon" -Filter "*.h" -Recurse
foreach ($header in $headers) {
    $content = Get-Content $header.FullName -Raw
    if ($content -match "void to_json\s*\(" -or $content -match "void from_json\s*\(") {
        Write-Host "WARNING: Inline JSON serialization found in $($header.Name)" -ForegroundColor Yellow
        Write-Host "  Move to corresponding .cpp file (see AGENTS.md rules)" -ForegroundColor Yellow
    }
}

Write-Host "`n=== Pre-commit Check PASSED ===" -ForegroundColor Green
