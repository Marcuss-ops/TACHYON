# Script to render various Tachyon backgrounds for comparison
# Duration: 5 seconds each
# Output: MP4 files in output/backgrounds/

$backgrounds = @(
    "tachyon.background.solid",
    "tachyon.background.linear_gradient",
    "tachyon.background.radial_gradient",
    "tachyon.background.image",
    "tachyon.background.video",
    "tachyon.background.kind.aura",
    "tachyon.background.kind.grid_modern",
    "tachyon.background.kind.stars",
    "tachyon.background.kind.galaxy",
    "tachyon.backgroundpreset.galaxy_premium",
    "tachyon.backgroundpreset.dark_tech_grid",
    "tachyon.backgroundpreset.cinematic_aura"
)

$tachyon = ".\build\src\RelWithDebInfo\tachyon.exe"
$out_dir = "output\backgrounds"

if (!(Test-Path $out_dir)) {
    New-Item -ItemType Directory -Path $out_dir
}

foreach ($bg in $backgrounds) {
    $env:TACHYON_BACKGROUND_ID = $bg
    $env:TACHYON_DURATION = "5"
    Write-Host "Rendering background: $bg"
    $out_file = Join-Path $out_dir "$bg.mp4"
    & $tachyon render --cpp labs\v2\backgrounds\background_demo.cpp --out $out_file
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "Failed to render $bg"
    }
}

Write-Host "Background rendering complete. Check output/backgrounds/ for results."