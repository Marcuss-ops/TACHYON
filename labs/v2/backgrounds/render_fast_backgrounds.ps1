$palettes = @(
    "dark_tech", "warm_amber", "cool_mint", "neon_night", "pure_white",
    "clean_white", "warm_white", "soft_white", "premium_dark", "neon_grid",
    "midnight_silk", "golden_horizon", "cyber_matrix", "frosted_glass",
    "cosmic_nebula", "brushed_metal", "oceanic_abyss", "royal_velvet", "prismatic_light"
)

$output_dir = "output/backgrounds"
if (-not (Test-Path $output_dir)) { New-Item -ItemType Directory -Path $output_dir }

$tachyon = ".\build\src\RelWithDebInfo\tachyon.exe"
$cpp_file = "labs/v2/backgrounds/all_palettes.cpp"

$original_content = Get-Content $cpp_file -Raw

foreach ($p in $palettes) {
    Write-Host "Rendering background with palette: $p"
    
    # Modify the palette name in the .cpp file
    $new_content = $original_content -replace '#define PALETTE_NAME ".*"', "#define PALETTE_NAME `"$p`""
    $new_content | Set-Content $cpp_file

    & $tachyon render --cpp $cpp_file --out "$output_dir/bg_$p.mp4" --frames 0-90
}

# Restore original content
$original_content | Set-Content $cpp_file

Write-Host "Background renders complete in $output_dir"
