# boundary-check.ps1 - Enforce 2D/3D domain boundaries
# Fail if renderer2d includes renderer3d or vice versa

$ErrorActionPreference = "Stop"

function Write-Check {
    param($Check, $Status, $Message = "")
    $color = if ($Status -eq "PASS") { "Green" } elseif ($Status -eq "FAIL") { "Red" } else { "Yellow" }
    Write-Host "  [$Status] " -ForegroundColor $color -NoNewline
    Write-Host "$Check" -NoNewline
    if ($Message) { Write-Host " - $Message" } else { Write-Host "" }
}

function Test-IncludeBoundary {
    param($Name, $Pattern, $AllowedPath)

    $violations = @()
    $files = Get-ChildItem -Path "include/tachyon/$AllowedPath" -Recurse -Filter "*.h" -ErrorAction SilentlyContinue

    foreach ($file in $files) {
        $content = Get-Content $file.FullName -Raw
        if ($content -match $Pattern) {
            $violations += $file.FullName
        }
    }

    return $violations
}

$totalErrors = 0

Write-Host "=== Tachyon Domain Boundary Check ===" -ForegroundColor Cyan
Write-Host ""

# Rule 1: renderer2d must not include renderer3d
Write-Host "Rule 1: renderer2d must not include renderer3d" -ForegroundColor Yellow
$violations1 = Test-IncludeBoundary -Name "2D->3D" -Pattern 'renderer3d|renderer3D' -AllowedPath "renderer2d"
if ($violations1.Count -eq 0) {
    Write-Check "renderer2d includes renderer3d" "PASS"
} else {
    Write-Check "renderer2d includes renderer3d" "FAIL" "$($violations1.Count) files violate"
    $violations1 | ForEach-Object { Write-Host "    $_" -ForegroundColor Red }
    $totalErrors++
}

# Rule 2: renderer3d must not include renderer2d (except interfaces)
Write-Host ""
Write-Host "Rule 2: renderer3d must not include renderer2d (except interfaces)" -ForegroundColor Yellow
$violations2 = Test-IncludeBoundary -Name "3D->2D" -Pattern 'renderer2d|renderer2D' -AllowedPath "renderer3d"
if ($violations2.Count -eq 0) {
    Write-Check "renderer3d includes renderer2d" "PASS"
} else {
    Write-Check "renderer3d includes renderer2d" "FAIL" "$($violations2.Count) files violate"
    $violations2 | ForEach-Object { Write-Host "    $_" -ForegroundColor Red }
    $totalErrors++
}

# Rule 3: text domain must not include scene_pack or renderer3d
Write-Host ""
Write-Host "Rule 3: text domain must not include scene_pack or renderer3d" -ForegroundColor Yellow
$violations3 = @()
$files3 = Get-ChildItem -Path "include/tachyon/text" -Recurse -Filter "*.h" -ErrorAction SilentlyContinue
foreach ($file in $files3) {
    $content = Get-Content $file.FullName -Raw
    if ($content -match 'scene_pack|renderer3d|renderer3D') {
        $violations3 += $file.FullName
    }
}
if ($violations3.Count -eq 0) {
    Write-Check "text domain includes forbidden headers" "PASS"
} else {
    Write-Check "text domain includes forbidden headers" "FAIL" "$($violations3.Count) files violate"
    $violations3 | ForEach-Object { Write-Host "    $_" -ForegroundColor Red }
    $totalErrors++
}

# Rule 4: presets/text must not include renderer3d or scene_pack
Write-Host ""
Write-Host "Rule 4: presets/text must not include renderer3d or scene_pack" -ForegroundColor Yellow
$violations4 = @()
$files4 = Get-ChildItem -Path "include/tachyon/presets" -Filter "*.h" -Recurse -ErrorAction SilentlyContinue
foreach ($file in $files4) {
    $content = Get-Content $file.FullName -Raw
    if ($content -match 'renderer3d|renderer3D|scene_pack') {
        $violations4 += $file.FullName
    }
}
if ($violations4.Count -eq 0) {
    Write-Check "presets domain includes forbidden headers" "PASS"
} else {
    Write-Check "presets domain includes forbidden headers" "FAIL" "$($violations4.Count) files violate"
    $violations4 | ForEach-Object { Write-Host "    $_" -ForegroundColor Red }
    $totalErrors++
}

# Rule 5: Check TACHYON_ENABLE_3D guards in concrete 3D types
Write-Host ""
Write-Host "Rule 5: Check TACHYON_ENABLE_3D guards in renderer2d concrete types" -ForegroundColor Yellow
$violations5 = @()
$files5 = Get-ChildItem -Path "src/renderer2d" -Recurse -Filter "*.cpp" -ErrorAction SilentlyContinue
foreach ($file in $files5) {
    $content = Get-Content $file.FullName -Raw
    if ($content -match 'RayTracer|ThreeDSpec|Mesh' -and $content -notmatch 'TACHYON_ENABLE_3D') {
        $violations5 += $file.FullName
    }
}
if ($violations5.Count -eq 0) {
    Write-Check "renderer2d concrete 3D types guarded" "PASS"
} else {
    Write-Check "renderer2d concrete 3D types guarded" "FAIL" "$($violations5.Count) files violate"
    $violations5 | ForEach-Object { Write-Host "    $_" -ForegroundColor Red }
    $totalErrors++
}

Write-Host ""
Write-Host "======================================" -ForegroundColor Cyan

if ($totalErrors -gt 0) {
    Write-Host "BOUNDARY CHECK FAILED: $totalErrors rule(s) violated" -ForegroundColor Red
    exit 1
} else {
    Write-Host "BOUNDARY CHECK PASSED" -ForegroundColor Green
    exit 0
}
