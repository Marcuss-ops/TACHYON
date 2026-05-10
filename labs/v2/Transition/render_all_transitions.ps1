# Batch render all main transitions for visual verification
$transitions = @(
    "tachyon.transition.crossfade",
    "tachyon.transition.wipe_linear",
    "tachyon.transition.slide",
    "tachyon.transition.circle_iris",
    "tachyon.transition.soft_zoom_blur",
    "tachyon.transition.flash_cut",
    "tachyon.transition.smooth_wipe",
    "tachyon.transition.push_left"
)

$outputDir = "output\transitions\clip_pair_demos"
if (-not (Test-Path $outputDir)) { New-Item -ItemType Directory -Force $outputDir }

foreach ($t in $transitions) {
    $name = $t.Split(".")[-1]
    $labFile = "labs\v2\Transition\temp_render_$name.cpp"
    $outFile = "$outputDir\transition_$name.mp4"
    
    Write-Host "Preparing render for: $t"
    
    # Generate temp lab file with the specific transition
    $template = Get-Content "labs\v2\Transition\transition_visual_lab.cpp" -Raw
    $content = $template -replace "tachyon.transition.crossfade", $t
    $content | Out-File -FilePath $labFile -Encoding utf8
    
    Write-Host "Rendering: $name ..."
    .\build\src\RelWithDebInfo\tachyon.exe render --cpp $labFile --out $outFile
    
    Remove-Item $labFile
}
