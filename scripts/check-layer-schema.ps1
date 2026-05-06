# check-layer-schema.ps1
# Enforce LayerSpec type migration by failing if 'kind' is used.

$found_kind = Get-ChildItem -Path "src", "include", "tests" -Recurse -Include *.cpp, *.h, *.hpp | 
    Select-String -Pattern "layer\.kind", "\.kind\s*=", "LayerSpec::kind"

if ($found_kind) {
    Write-Error "ERROR: Legacy 'kind' usage found in LayerSpec schema!"
    $found_kind | Out-String | Write-Host
    exit 1
}

Write-Host "SUCCESS: No legacy 'kind' usage found."
exit 0
