#!/usr/bin/env pwsh

$text_dirs = @("src/text", "include/tachyon/text")
$forbidden = @("renderer2d/", "renderer3d/", "scene3d")

Write-Host "Checking Text Boundary..."
$violation = $false

foreach ($dir in $text_dirs) {
    if (-Not (Test-Path $dir)) { continue }
    $files = Get-ChildItem -Path $dir -Recurse -Include *.cpp, *.h, *.hpp
    foreach ($file in $files) {
        $content = Get-Content $file.FullName
        foreach ($inc in $forbidden) {
            if ($content -match "#include.*$inc") {
                Write-Host "!! Boundary Violation: $($file.FullName) includes $inc" -ForegroundColor Red
                $violation = $true
            }
        }
    }
}

if ($violation) {
    Write-Host "Boundary Check Failed!" -ForegroundColor Red
    exit 1
} else {
    Write-Host "Text Boundary Check Passed." -ForegroundColor Green
    exit 0
}
