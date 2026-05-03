#include "tachyon/scene/builder.h"
#include "tachyon/runtime/execution/native_render.h"
#include "tachyon/presets/background/background_preset_registry.h"
#include <iostream>
#include <array>
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

    std::cout << "[NativeRender] Rendering light leak transition demos...\n";
    struct DemoConfig {
        const char* scene_id;
        double preset;
        const char* file_name;
    };

    const std::array<DemoConfig, 3> light_leak_demos{{
        {"light_leak_demo", 0.0, "light_leak_demo.mp4"},
        {"film_burn_demo", 1.0, "film_burn_demo.mp4"},
        {"flash_demo", 7.0, "flash_demo.mp4"},
    }};
    
    const std::filesystem::path demo_dir = std::filesystem::current_path() / "tests" / "output" / "light_leaks";
    std::filesystem::create_directories(demo_dir);

    for (const auto& demo : light_leak_demos) {
        tachyon::LayerSpec layer;
        layer.id = "fx";
        layer.name = "Transition FX";
        layer.type = "solid";
        layer.kind = tachyon::LayerType::Solid;
        layer.width = 1280;
        layer.height = 720;
        layer.start_time = 0.0;
        layer.in_point = 0.0;
        layer.out_point = 3.5;
        layer.opacity = 1.0;
        layer.transform.position_x = 0.0;
        layer.transform.position_y = 0.0;
        layer.transform.scale_x = 1.0;
        layer.transform.scale_y = 1.0;
        // Keep the leak layer opaque and black so the FX stays separate from the background.
        layer.fill_color.value = {0, 0, 0, 255};

        tachyon::AnimatedEffectSpec effect;
        effect.type = "light_leak";
        effect.static_scalars["preset"] = demo.preset;
        effect.static_scalars["speed"] = 1.0;
        effect.static_scalars["seed"] = 3.0;
        effect.animated_scalars["progress"].keyframes.push_back({0.0, 0.0});
        effect.animated_scalars["progress"].keyframes.push_back({0.5, 0.0});
        effect.animated_scalars["progress"].keyframes.push_back({3.0, 1.0});
        effect.animated_scalars["progress"].keyframes.push_back({3.5, 1.0});
        layer.animated_effects.push_back(std::move(effect));

        tachyon::CompositionSpec comp;
        comp.id = "main";
        comp.name = "Main";
        comp.width = 1280;
        comp.height = 720;
        comp.duration = 3.5;
        comp.frame_rate = {30, 1};
        comp.background = tachyon::BackgroundSpec::from_string("#000000");
        tachyon::LayerSpec background_layer;
        background_layer.id = "bg";
        background_layer.name = "Background";
        background_layer.type = "solid";
        background_layer.kind = tachyon::LayerType::Solid;
        background_layer.width = 1280;
        background_layer.height = 720;
        background_layer.opacity = 1.0;
        background_layer.fill_color.value = {0, 0, 0, 255};

        comp.layers.push_back(background_layer);
        comp.layers.push_back(layer);

        tachyon::SceneSpec scene;
        scene.project.id = std::string(demo.scene_id) + "_v2";
        scene.project.name = std::string(demo.scene_id) + "_v2";
        scene.compositions.push_back(comp);

        tachyon::RenderJob job;
        job.job_id = std::string(demo.scene_id) + "_v2";
        job.scene_ref = std::string(demo.scene_id) + "_v2";
        job.composition_target = "main";
        job.frame_range = {0, 105};
        job.output.destination.path = (demo_dir / demo.file_name).string();
        job.output.destination.overwrite = true;
        job.output.profile.name = "h264-mp4";
        job.output.profile.class_name = "video";
        job.output.profile.container = "mp4";
        job.output.profile.alpha_mode = "opaque";
        job.output.profile.video.codec = "libx264";
        job.output.profile.video.pixel_format = "yuv420p";
        job.output.profile.video.rate_control_mode = "crf";
        job.output.profile.video.crf = 23.0;
        job.output.profile.color.space = "bt709";
        job.output.profile.color.transfer = "srgb";
        job.output.profile.color.range = "full";

        std::filesystem::remove(job.output.destination.path);
        const auto result = NativeRenderer::render(scene, job);
        if (!result.output_error.empty()) {
            std::cerr << "[NativeRender] FAIL: " << demo.scene_id << ": " << result.output_error << "\n";
            return false;
        }
        std::cout << "[OK] Rendered " << demo.scene_id << " to " << (demo_dir / demo.file_name) << "\n";
    }
    return true;
}

} // namespace tachyon
