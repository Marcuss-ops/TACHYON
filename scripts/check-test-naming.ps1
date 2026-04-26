# Test Naming Convention Checker for Tachyon
# Ensures tests follow discoverable naming patterns

param(
    [string]$TestDir = "tests",
    [switch]$Fix
)

$ErrorActionPreference = 'Stop'

Write-Host "=== Test Naming Convention Check ===" -ForegroundColor Cyan

# Define naming patterns
$patterns = @{
    "Unit tests" = "^unit/.*_tests\.cpp$"
    "Integration tests" = "^integration/.*_tests?\.cpp$"
    "Compile tests" = "^compile/header_smoke_.*\.cpp$"
    "Component tests" = ".*component.*_tests?\.cpp$"
    "Spec tests" = ".*spec.*_tests?\.cpp$"
    "Evaluator tests" = ".*evaluator.*_tests?\.cpp$"
}

# Get all test files
$testFiles = Get-ChildItem -Path $TestDir -Filter "*.cpp" -Recurse | Where-Object { $_.FullName -notmatch "test_main" }
$allGood = $true

Write-Host "`nChecking $($testFiles.Count) test files..." -ForegroundColor Yellow

foreach ($testFile in $testFiles) {
    $relativePath = $testFile.FullName.Substring((Get-Location).Path.Length + 1)
    $fileName = $testFile.Name
    $isGood = $false
    
    # Check if file matches any pattern
    foreach ($pattern in $patterns.Values) {
        if ($relativePath -match $pattern) {
            $isGood = $true
            break
        }
    }
    
    # Special check: file should end with "tests.cpp" or "test.cpp"
    if (-not $isGood) {
        if ($fileName -match "_tests?\.cpp$" -or $fileName -match "test.*\.cpp$") {
            $isGood = $true
        }
    }
    
    if (-not $isGood) {
        Write-Host "  WARNING: $relativePath" -ForegroundColor Yellow
        Write-Host "    Does not follow naming convention (should be *_tests.cpp)" -ForegroundColor Yellow
        $allGood = $false
    }
}

# Check for tests without corresponding implementation
Write-Host "`nChecking for missing test coverage..." -ForegroundColor Yellow

# Get all headers
$headers = Get-ChildItem -Path "include/tachyon" -Filter "*.h" -Recurse
$testNames = $testFiles | ForEach-Object { $_.Name -replace "\.cpp$", "" }

$missingTests = @()
foreach ($header in $headers) {
    $headerName = $header.BaseName
    # Skip pch.h, api.h, etc.
    if ($headerName -match "^(pch|api|c_api)$") { continue }
    
    # Check if there's a corresponding test
    $hasTest = $false
    foreach ($testName in $testNames) {
        if ($testName -match $headerName) {
            $hasTest = $true
            break
        }
    }
    
    if (-not $hasTest) {
        $missingTests += $headerName
    }
}

if ($missingTests.Count -gt 0) {
    Write-Host "  Info: $($missingTests.Count) headers without obvious test coverage" -ForegroundColor Cyan
    if ($missingTests.Count -le 10) {
        foreach ($missing in $missingTests) {
            Write-Host "    - $missing" -ForegroundColor Cyan
        }
    }
}

# Check test registration in CMakeLists.txt
Write-Host "`nChecking test registration in CMakeLists.txt..." -ForegroundColor Yellow
$cmakeContent = Get-Content "tests/CMakeLists.txt" -Raw

$unregistered = @()
foreach ($testFile in $testFiles) {
    $relativePath = $testFile.FullName.Substring((Get-Location).Path.Length + 1)
    if ($cmakeContent -notmatch [regex]::Escape($relativePath)) {
        $unregistered += $relativePath
    }
}

if ($unregistered.Count -gt 0) {
    Write-Host "  ERROR: $($unregistered.Count) test files not registered in CMakeLists.txt:" -ForegroundColor Red
    foreach ($file in $unregistered) {
        Write-Host "    - $file" -ForegroundColor Red
    }
    $allGood = $false
} else {
    Write-Host "  All test files are registered in CMakeLists.txt" -ForegroundColor Green
}

# Summary
Write-Host "`n=== Summary ===" -ForegroundColor Cyan
if ($allGood) {
    Write-Host "All tests follow naming conventions!" -ForegroundColor Green
    exit 0
} else {
    Write-Host "Some tests need attention. See warnings above." -ForegroundColor Yellow
    exit 1
}
