#include "tachyon/scene/builder.h"
#include "tachyon/runtime/execution/native_render.h"
#include "tachyon/presets/background/background_preset_registry.h"
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

namespace tachyon {

bool run_native_render_tests() {
    using namespace scene;

    std::cout << "[NativeRender] Starting Premium Background Batch Render...\n";

    for (const auto& pid : presets::list_background_presets()) {
        std::cout << "[NativeRender] Rendering: " << pid << "...\n";
        
        auto bg_opt = presets::build_background_preset(pid, 1280, 720);
        if (!bg_opt) continue;
        LayerSpec bg = *bg_opt;

        auto scene = Composition(pid)
            .size(1280, 720)
            .duration(1.0)
            .fps(30)
            .layer("bg", [&](LayerBuilder& l) {
                l = LayerBuilder(bg);
            })
            .build_scene();

        std::filesystem::path out_path = std::filesystem::temp_directory_path() / "tachyon_tests" / (pid + ".mp4");
        std::filesystem::create_directories(out_path.parent_path());
        
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
