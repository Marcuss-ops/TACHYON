#!/usr/bin/env pwsh

$renderer2d = "src/renderer2d"
$forbidden = @("renderer3d/", "embree3/", "raytracer/", "embree/")

Write-Host "Checking 2D-3D Boundary..."
$violation = $false

$files = Get-ChildItem -Path $renderer2d -Recurse -Include *.cpp, *.h, *.hpp
foreach ($file in $files) {
    $content = Get-Content $file.FullName
    foreach ($inc in $forbidden) {
        if ($content -match "#include.*$inc") {
            Write-Host "!! Boundary Violation: $($file.FullName) includes $inc" -ForegroundColor Red
            $violation = $true
        }
    }
}

if ($violation) {
    Write-Host "Boundary Check Failed!" -ForegroundColor Red
    exit 1
} else {
    Write-Host "2D-3D Boundary Check Passed." -ForegroundColor Green
    exit 0
}
