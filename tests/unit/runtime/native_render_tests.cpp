#include "tachyon/scene/builder.h"
#include "tachyon/runtime/execution/native_render.h"
#include "tachyon/presets/background/procedural.h"
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

namespace tachyon {

bool run_native_render_tests() {
    using namespace scene;
    using namespace presets::background;

    std::cout << "[NativeRender] Starting Premium Background Batch Render...\n";

    std::vector<std::string> presets = {
        "midnight_silk", "golden_horizon", "cyber_matrix", "frosted_glass",
        "cosmic_nebula", "brushed_metal", "oceanic_abyss", "royal_velvet",
        "prismatic_light", "technical_blueprint"
    };

    for (const auto& pid : presets) {
        std::cout << "[NativeRender] Rendering: " << pid << "...\n";
        
        LayerSpec bg;
        if (pid == "midnight_silk") bg = procedural_bg::midnight_silk(1280, 720);
        else if (pid == "golden_horizon") bg = procedural_bg::golden_horizon(1280, 720);
        else if (pid == "cyber_matrix") bg = procedural_bg::cyber_matrix(1280, 720);
        else if (pid == "frosted_glass") bg = procedural_bg::frosted_glass(1280, 720);
        else if (pid == "cosmic_nebula") bg = procedural_bg::cosmic_nebula(1280, 720);
        else if (pid == "brushed_metal") bg = procedural_bg::brushed_metal(1280, 720);
        else if (pid == "oceanic_abyss") bg = procedural_bg::oceanic_abyss(1280, 720);
        else if (pid == "royal_velvet") bg = procedural_bg::royal_velvet(1280, 720);
        else if (pid == "prismatic_light") bg = procedural_bg::prismatic_light(1280, 720);
        else if (pid == "technical_blueprint") bg = procedural_bg::technical_blueprint(1280, 720);

        auto scene = Composition(pid)
            .size(1280, 720)
            .duration(1.0)
            .fps(30)
            .layer("bg", [&](LayerBuilder& l) {
                l = LayerBuilder(bg);
            })
            .build_scene();

        std::filesystem::path out_path = std::filesystem::path("c:/Users/pater/Pyt/Tachyon/output") / (pid + ".mp4");
        
        RenderJob job;
        job.job_id = "test_" + pid;
        job.composition_target = pid;
        job.frame_range = {0, 30};
        job.output.destination.path = out_path.string();
        job.output.destination.overwrite = true;
        job.output.profile.container = "mp4";
        job.output.profile.video.codec = "libx264";
        job.output.profile.video.pixel_format = "yuv420p";

        const auto result = NativeRenderer::render(scene, job);
        if (!result.output_error.empty()) {
            std::cerr << "FAIL: Failed rendering " << pid << ": " << result.output_error << "\n";
            return false;
        }
        std::cout << "[OK] Rendered " << pid << " to " << out_path << "\n";
    }
    return true;
}

} // namespace tachyon
