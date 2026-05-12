#include "tachyon/core/spec/cpp_scene_loader.h"
#include "tachyon/presets/background/fluent.h"
#include "tachyon/runtime/runtime_facade.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

std::filesystem::path make_temp_dir() {
    auto dir = std::filesystem::temp_directory_path() / "tachyon_jit_render_tests";
    std::filesystem::create_directories(dir);
    return dir;
}


std::size_t count_visible_pixels(const tachyon::renderer2d::Framebuffer& frame) {
    std::size_t count = 0;
    for (std::uint32_t y = 0; y < frame.height(); ++y) {
        for (std::uint32_t x = 0; x < frame.width(); ++x) {
            const auto pixel = frame.get_pixel(x, y);
            if (pixel.r > 0.05f || pixel.g > 0.05f || pixel.b > 0.05f || pixel.a > 0.05f) {
                ++count;
            }
        }
    }
    return count;
}

std::filesystem::path write_jit_abi_scene_source(const std::filesystem::path& dir) {
    const auto cpp_path = dir / "jit_abi_render_scene.cpp";
    std::ofstream out(cpp_path);
    out << R"cpp(
#include "tachyon/jit/tachyon_jit_api.h"

extern "C" TACHYON_JIT_API int tachyon_jit_build_scene(const TachyonHostApi* host, TachyonSceneHandle scene) {
    if (!host || host->abi_version != TACHYON_JIT_ABI_VERSION) {
        return TACHYON_JIT_ERROR_ABI_MISMATCH;
    }

    host->begin_scene(host->ctx, scene);
    
    // Background - use a solid color for robustness in test
    TachyonLayerHandle bg = host->add_background(host->ctx, scene, "solid_bg");
    if (bg) {
        host->set_float(host->ctx, bg, "opacity", 1.0f);
        // We don't have a set_color in TachyonHostApi yet, 
        // but solid should default to white or we can use set_float for components if supported.
        // For now, let's just see if it's not black.
    }

    // Text
    host->add_text(host->ctx, scene, "JIT ABI Works!", 960.0f, 540.0f);

    host->end_scene(host->ctx, scene);
    
    return TACHYON_JIT_OK;
}

extern "C" TACHYON_JIT_API const TachyonJitManifest* tachyon_jit_get_manifest() {
    static TachyonJitManifest manifest = {
        TACHYON_JIT_ABI_VERSION,
        sizeof(TachyonJitManifest),
        "Test ABI Plugin",
        "1.0.0"
    };
    return &manifest;
}
)cpp";
    return cpp_path;
}

} // namespace

bool run_jit_render_tests() {
    using namespace tachyon;

    g_failures = 0;

    const auto temp_dir = make_temp_dir();
    

    // Test JIT C ABI
    {
        const auto cpp_path = write_jit_abi_scene_source(temp_dir);
        const auto loaded = CppSceneLoader::load_from_file(cpp_path, temp_dir / "build_abi");
        check_true(loaded.success, "New JIT ABI scene should compile and load");
        if (loaded.success && loaded.scene.has_value()) {
            const auto compiled = RuntimeFacade::instance().compile_scene(*loaded.scene);
            check_true(compiled.ok(), "New JIT ABI compiled scene should be valid");
            if (compiled.ok() && compiled.value.has_value()) {
                const auto rendered = RuntimeFacade::instance().render_frame(*compiled.value, 0);
                check_true(rendered.ok(), "New JIT ABI scene should render");
                if (rendered.ok() && rendered.value.has_value() && rendered.value->frame) {
                    check_true(count_visible_pixels(*rendered.value->frame) > 0, "New JIT ABI frame should not be black");
                }
            }
        }
    }

    return g_failures == 0;
}
