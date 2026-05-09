$typewriters = @(
    @{ Name = "typewriter_01_basic";   Scene = "labs\v2\Typewriter\typewriter_01_basic.cpp" },
    @{ Name = "typewriter_02_smooth";   Scene = "labs\v2\Typewriter\typewriter_02_smooth.cpp" },
    @{ Name = "typewriter_03_word";     Scene = "labs\v2\Typewriter\typewriter_03_word.cpp" },
    @{ Name = "typewriter_04_blur";     Scene = "labs\v2\Typewriter\typewriter_04_blur.cpp" },
    @{ Name = "typewriter_05_slide";    Scene = "labs\v2\Typewriter\typewriter_05_slide.cpp" },
    @{ Name = "typewriter_06_pop";      Scene = "labs\v2\Typewriter\typewriter_06_pop.cpp" },
    @{ Name = "typewriter_07_fast";     Scene = "labs\v2\Typewriter\typewriter_07_fast.cpp" },
    @{ Name = "typewriter_08_overlap";  Scene = "labs\v2\Typewriter\typewriter_08_overlap.cpp" },
    @{ Name = "typewriter_09_stagger_blur"; Scene = "labs\v2\Typewriter\typewriter_09_stagger_blur.cpp" },
    @{ Name = "typewriter_10_combo";    Scene = "labs\v2\Typewriter\typewriter_10_combo.cpp" }
)

$tachyon = ".\build\src\RelWithDebInfo\tachyon.exe"
$out_dir = "output\typewriter"

if (!(Test-Path $out_dir)) {
    New-Item -ItemType Directory -Path $out_dir | Out-Null
}

foreach ($demo in $typewriters) {
    Write-Host "Rendering typewriter demo: $($demo.Name)"
    $out_file = Join-Path $out_dir "$($demo.Name).mp4"
    & $tachyon render --cpp $demo.Scene --out $out_file
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "Failed to render $($demo.Name)"
    }
}

Write-Host "Typewriter rendering complete. Check output/typewriter/ for results."
