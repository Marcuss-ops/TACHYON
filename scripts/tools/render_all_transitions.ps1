$transitions = @(
    "crossfade",
    "slide",
    "slide_up",
    "swipe_left",
    "zoom",
    "flip",
    "blur",
    "fade_to_black",
    "wipe_linear",
    "wipe_angular",
    "push_left",
    "slide_easing",
    "circle_iris",
    "glitch_slice",
    "pixelate",
    "rgb_split",
    "kaleidoscope",
    "ripple",
    "spin",
    "luma_dissolve",
    "zoom_blur",
    "zoom_in",
    "directional_blur_wipe",
    "light_leak",
    "film_burn"
)

$tachyon = ".\build\src\RelWithDebInfo\tachyon.exe"
$out_dir = "output\transitions"

if (!(Test-Path $out_dir)) {
    New-Item -ItemType Directory -Path $out_dir
}

foreach ($t in $transitions) {
    $env:TACHYON_TRANSITION_ID = $t
    Write-Host "Rendering transition: $t"
    $out_file = Join-Path $out_dir "$t.mp4"
    & $tachyon render --cpp labs\transition_demo.cpp --out $out_file
}
