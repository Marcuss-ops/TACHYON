# primitive-check.ps1 - Enforce Renderer2D primitives ownership
# Fail if forbidden patterns appear outside owner domains

$ErrorActionPreference = "Stop"

function Write-Check {
    param($Check, $Status, $Message = "")
    $color = if ($Status -eq "PASS") { "Green" } elseif ($Status -eq "FAIL") { "Red" } else { "Yellow" }
    Write-Host "  [$Status] " -ForegroundColor $color -NoNewline
    Write-Host "$Check" -NoNewline
    if ($Message) { Write-Host " - $Message" } else { Write-Host "" }
}

function Test-PatternOutsideDomain {
    param($Pattern, $OwnerDomain, $PatternName)

    $violations = @()
    $allFiles = @()
    $allFiles += Get-ChildItem -Path "src" -Recurse -Filter "*.cpp" -ErrorAction SilentlyContinue
    $allFiles += Get-ChildItem -Path "src" -Recurse -Filter "*.h" -ErrorAction SilentlyContinue
    $allFiles += Get-ChildItem -Path "include" -Recurse -Filter "*.h" -ErrorAction SilentlyContinue
    $allowlistPatterns = @()

    switch ($PatternName) {
        "gaussian_blur" {
            $allowlistPatterns = @(
                'src\\renderer2d\\backend\\',
                'include\\tachyon\\renderer2d\\backend\\',
                'src\\text\\core\\rendering\\',
                'include\\tachyon\\text\\rendering\\'
            )
        }
        "blend" {
            $allowlistPatterns = @(
                'src\\color\\',
                'include\\tachyon\\color\\',
                'src\\renderer2d\\color\\',
                'include\\tachyon\\renderer2d\\color\\'
            )
        }
        "sampling" {
            $allowlistPatterns = @(
                'src\\renderer2d\\effects\\',
                'src\\timeline\\'
            )
        }
    }

    foreach ($file in $allFiles) {
        # Skip files in the owner domain
        if ($file.FullName -match [regex]::Escape($OwnerDomain)) {
            continue
        }

        $skip = $false
        foreach ($allowPattern in $allowlistPatterns) {
            if ($file.FullName -match $allowPattern) {
                $skip = $true
                break
            }
        }
        if ($skip) {
            continue
        }

        # Check for allowlist comment
        $content = Get-Content $file.FullName -Raw
        if ($content -match "PRIMITIVE_ALLOWLIST.*$PatternName") {
            continue
        }

        # Check for pattern
        if ($content -match $Pattern) {
            $violations += $file.FullName
        }
    }

    return $violations
}

$totalErrors = 0

Write-Host "=== Renderer2D Primitives Ownership Check ===" -ForegroundColor Cyan
Write-Host ""

# Rule 1: gaussian_blur must be in renderer2d/effects
Write-Host "Rule 1: gaussian_blur must be in renderer2d/effects" -ForegroundColor Yellow
$violations1 = Test-PatternOutsideDomain -Pattern "gaussian_blur" -OwnerDomain "renderer2d/effects" -PatternName "gaussian_blur"
if ($violations1.Count -eq 0) {
    Write-Check "gaussian_blur ownership" "PASS"
} else {
    Write-Check "gaussian_blur ownership" "FAIL" "$($violations1.Count) files violate"
    $violations1 | ForEach-Object { Write-Host "    $_" -ForegroundColor Red }
    $totalErrors++
}

# Rule 2: blend modes must be in renderer2d/blend
Write-Host ""
Write-Host "Rule 2: blend modes must be in renderer2d/blend" -ForegroundColor Yellow
$violations2 = Test-PatternOutsideDomain -Pattern "blend_normal|blend_multiply|blend_screen" -OwnerDomain "renderer2d/blend" -PatternName "blend"
if ($violations2.Count -eq 0) {
    Write-Check "blend modes ownership" "PASS"
} else {
    Write-Check "blend modes ownership" "FAIL" "$($violations2.Count) files violate"
    $violations2 | ForEach-Object { Write-Host "    $_" -ForegroundColor Red }
    $totalErrors++
}

# Rule 3: composite operations must be in renderer2d/composite
Write-Host ""
Write-Host "Rule 3: composite operations must be in renderer2d/composite" -ForegroundColor Yellow
$violations3 = Test-PatternOutsideDomain -Pattern "alpha_over|composite_over" -OwnerDomain "renderer2d/composite" -PatternName "composite"
if ($violations3.Count -eq 0) {
    Write-Check "composite operations ownership" "PASS"
} else {
    Write-Check "composite operations ownership" "FAIL" "$($violations3.Count) files violate"
    $violations3 | ForEach-Object { Write-Host "    $_" -ForegroundColor Red }
    $totalErrors++
}

# Rule 4: sampling operations must be in renderer2d/sampling
Write-Host ""
Write-Host "Rule 4: sampling operations must be in renderer2d/sampling" -ForegroundColor Yellow
$violations4 = Test-PatternOutsideDomain -Pattern "sample_bilinear|sample_nearest" -OwnerDomain "renderer2d/sampling" -PatternName "sampling"
if ($violations4.Count -eq 0) {
    Write-Check "sampling operations ownership" "PASS"
} else {
    Write-Check "sampling operations ownership" "FAIL" "$($violations4.Count) files violate"
    $violations4 | ForEach-Object { Write-Host "    $_" -ForegroundColor Red }
    $totalErrors++
}

Write-Host ""
Write-Host "==========================================" -ForegroundColor Cyan

if ($totalErrors -gt 0) {
    Write-Host "PRIMITIVES CHECK FAILED: $totalErrors rule(s) violated" -ForegroundColor Red
    exit 1
} else {
    Write-Host "PRIMITIVES CHECK PASSED" -ForegroundColor Green
    exit 0
}
