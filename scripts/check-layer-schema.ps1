# Check for remaining layer.kind references that should be removed
# This script fails if it finds LayerSpec::kind or layer.kind usage

Write-Host "Checking for remaining layer.kind references..." -ForegroundColor Cyan

$ErrorActionPreference = "Stop"

# Check for LayerSpec::kind in headers and source
$files = Get-ChildItem -Path src/, include/, tests/, examples/ -Recurse -Include *.cpp,*.h,*.hpp -ErrorAction SilentlyContinue

$issues = @()
foreach ($file in $files) {
    $content = Get-Content $file.FullName -ErrorAction SilentlyContinue
    foreach ($line in $content) {
        if ($line -match "LayerSpec.*kind|layer\.kind|\.kind.*LayerSpec") {
            $issues += "$($file.FullName): $line"
        }
    }
}

if ($issues.Count -gt 0) {
    Write-Host "FAIL: Found remaining layer.kind references:" -ForegroundColor Red
    $issues | ForEach-Object { Write-Host $_ -ForegroundColor Yellow }
    exit 1
}

# Check layer_spec.h for 'kind' field (should use type/type_string instead)
$layerSpecFile = "include/tachyon/core/spec/schema/objects/layer_spec.h"
if (Test-Path $layerSpecFile) {
    $content = Get-Content $layerSpecFile
    $kindFields = $content | Select-String -Pattern "^\s*.*\bkind\b.*;" | Where-Object { $_ -notmatch "transition_in|transition_out|LayerTransition" }

    if ($kindFields) {
        Write-Host "WARNING: Found 'kind' field in layer_spec.h:" -ForegroundColor Yellow
        $kindFields | ForEach-Object { Write-Host $_ -ForegroundColor Yellow }
        Write-Host "Note: layer.kind should not exist; use type (enum) and type_string instead" -ForegroundColor Yellow
    }
}

Write-Host "OK: No problematic layer.kind references found" -ForegroundColor Green
exit 0
