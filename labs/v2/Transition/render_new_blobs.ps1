# Render script for premium blob transitions
$transitions = @(
    "tachyon.transition.lightleak.lava_flow",
    "tachyon.transition.lightleak.liquid_fission",
    "tachyon.transition.lightleak.cosmic_swirl"
)

$outputDir = "output\transitions\premium_blobs"
if (-not (Test-Path $outputDir)) { New-Item -ItemType Directory -Force $outputDir }

# Premium minimalist template: one solid constant background 
$template = @"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/scene/builder.h"

using namespace tachyon;
using namespace tachyon::scene;

extern "C" __declspec(dllexport) SceneSpec build_scene() {
    SceneBuilder scene;
    scene.composition("temp_comp", [](CompositionBuilder &comp) {
        comp.size(1280, 720).fps(30).duration(5.0);
        
        // Layer 1: Permanent Deep Charcoal Background
        comp.layer("base_field", [](LayerBuilder &l) {
            l.solid("charcoal").color({25, 25, 27, 255}).in(0.0).out(5.0);
        });
        
        // Layer 2: Identical color to test the effect purely as a seamless cinematic overlay
        comp.layer("transition_field", [](LayerBuilder &l) {
            l.solid("charcoal_overlay").color({25, 25, 27, 255}).in(1.5).out(5.0);
            
            // Premium slowed-down effect duration of 2.0 seconds instead of 1.5
            l.enter().id("TRANSITION_ID_PLACEHOLDER").duration(2.0);
        });
    });
    return scene.build();
}
"@

foreach ($t in $transitions) {
    $name = $t.Split(".")[-1]
    $labFile = "labs\v2\Transition\temp_premium_$name.cpp"
    $outFile = "$outputDir\premium_$name.mp4"
    
    Write-Host "Processing premium $name ..."
    $content = $template.Replace("TRANSITION_ID_PLACEHOLDER", $t)
    $content | Out-File -FilePath $labFile -Encoding utf8 -NoNewline
    
    Write-Host "Rendering to $outFile ..."
    # Using full build/src path to ensure we use the recompiled executable
    & ".\build\src\RelWithDebInfo\tachyon.exe" render --cpp $labFile --out $outFile
    
    Remove-Item $labFile
}

Write-Host "All premium renders complete in $outputDir"
