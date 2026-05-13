# Check for duplicate primitives outside their owner domains
# This prevents text/background/transitions from reinventing blend/sampling/blur

param(
    [string]$Path = ".",
    [switch]$Verbose
)

Write-Host "Checking for primitive ownership violations..." -ForegroundColor Cyan

$ErrorActionPreference = "Stop"

# Define primitive ownership
# These primitives should only appear in their owner domains
$primitiveOwners = @{
    "gaussian_blur" = @("renderer2d", "effects")
    "blend_normal" = @("renderer2d")
    "blend_multiply" = @("renderer2d")
    "alpha_over" = @("renderer2d")
    "composite_over" = @("renderer2d")
    "sample_bilinear" = @("renderer2d")
    "sample_nearest" = @("renderer2d")
}

$issues = @()
$files = Get-ChildItem -Path $Path/src/, $Path/include/ -Recurse -Include *.cpp,*.h,*.hpp -ErrorAction SilentlyContinue

foreach ($file in $files) {
    $relativePath = $file.FullName.Replace($Path, "").TrimStart('/', '\\')
    $content = Get-Content $file.FullName -ErrorAction SilentlyContinue

    foreach ($primitive in $primitiveOwners.Keys) {
        $allowedDomains = $primitiveOwners[$primitive]

        # Check if file is in allowed domain
        $inAllowedDomain = $false
        foreach ($domain in $allowedDomains) {
            if ($relativePath -match [regex]::Escape($domain)) {
                $inAllowedDomain = $true
                break
            }
        }

        if (-not $inAllowedDomain) {
            # Check if primitive is used in this file
            $lineNum = 0
            foreach ($line in $content) {
                $lineNum++
                if ($line -match [regex]::Escape($primitive)) {
                    $issues += "Primitive '$primitive' used outside owner domain in: $relativePath (line $lineNum)"
                }
            }
        }
    }
}

if ($issues.Count -gt 0) {
    Write-Host "FAIL: Found primitive ownership violations:" -ForegroundColor Red
    $issues | ForEach-Object { Write-Host $_ -ForegroundColor Yellow }
    exit 1
}

Write-Host "OK: No primitive ownership violations found" -ForegroundColor Green
exit 0
