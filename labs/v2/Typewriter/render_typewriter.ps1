$ErrorActionPreference = "Continue"

$demos = @(
    "typewriter_01_basic",
    "typewriter_02_smooth",
    "typewriter_03_word",
    "typewriter_04_blur",
    "typewriter_05_slide",
    "typewriter_06_pop",
    "typewriter_07_overlap",
    "typewriter_08_stagger_blur",
    "typewriter_09_scale",
    "typewriter_10_glow",
    "typewriter_11_tilt"
)

$tachyon = ".\build\src\RelWithDebInfo\tachyon.exe"
$output_dir = "output\typewriter"

if (-not (Test-Path $output_dir)) {
    New-Item -ItemType Directory -Path $output_dir -Force
}

foreach ($demo in $demos) {
    Write-Host "Rendering typewriter demo: $demo" -ForegroundColor Cyan
    $cpp_file = "labs\v2\Typewriter\$demo.cpp"
    $out_file = "$output_dir\$demo.mp4"
    
    if (Test-Path $cpp_file) {
        & $tachyon render --cpp $cpp_file --out $out_file
        if ($LASTEXITCODE -ne 0) {
            Write-Host "WARNING: Failed to render $demo" -ForegroundColor Red
        }
    } else {
        Write-Host "ERROR: C++ file does not exist: $cpp_file" -ForegroundColor Red
    }
}

Write-Host "Typewriter rendering complete. Check $output_dir for results." -ForegroundColor Green
