#include "test_utils.h"
#include "tachyon/runtime/plugins/plugin_manager.h"
#include "tachyon/core/spec/authoring_service.h"
#include "tachyon/transition_registry.h"
#include "tachyon/renderer2d/core/framebuffer.h"

#include <iostream>
#include <fstream>
#include <filesystem>

bool run_plugin_tests() {
    std::cout << "[PluginTests] Starting Minimal Plugin ABI unit tests...\n";
    using namespace tachyon;
    using namespace tachyon::runtime;
    using namespace tachyon::renderer2d;
    using namespace tachyon::core::spec;

    // 1. Create a dynamic plugin C++ source file on-the-fly
    std::string plugin_src = R"cpp(
#include "tachyon/runtime/plugins/plugin_abi.h"

extern "C" {

static TachyonPluginColor test_custom_wipe(
    float u, float v, float t,
    TachyonPluginSurfaceHandle input_surface,
    TachyonPluginSurfaceHandle to_surface
) {
    // Basic gradient formula for deterministic testing
    float r = u * t;
    float g = v * t;
    float b = t;
    float a = 1.0f;
    return TachyonPluginColor{r, g, b, a};
}

static const TachyonPluginManifest test_manifest = {
    "wipe_plugin",
    "Tachyon Core Team",
    "A custom wipe transition plugin for Tachyon",
    1, 0, 0
};

const TachyonPluginManifest* tachyon_plugin_get_manifest() {
    return &test_manifest;
}

int tachyon_plugin_init(const TachyonPluginApi* api) {
    if (!api) return -1;
    return api->register_transition(
        api->registry_handle,
        "plugin.test_wipe",
        "Plugin Wipe",
        test_custom_wipe
    );
}

}
)cpp";

    std::filesystem::path src_path = "plugin_test_src.cpp";
    std::filesystem::path out_dir = "build_plugin_test";
    std::filesystem::create_directories(out_dir);

    {
        std::ofstream out(src_path);
        out << plugin_src;
    }

    // 2. Compile the plugin to a dynamic library
    std::cout << "[PluginTests] Compiling test plugin on-the-fly...\n";
    auto compile_res = AuthoringService::compile_to_shared_lib(src_path, out_dir);
    
    // Cleanup source file
    std::filesystem::remove(src_path);

    if (!compile_res.success) {
        std::cerr << "[PluginTests] FAIL: On-the-fly compilation failed: " << compile_res.error << "\n";
        return false;
    }

    std::cout << "[PluginTests] Dynamic plugin compiled successfully to: " << compile_res.output_path << "\n";

    // 3. Load the plugin using PluginManager
    TransitionRegistry registry;
    PluginManager& pm = PluginManager::get_instance();

    // Verify loading works
    bool loaded = pm.load_plugin(compile_res.output_path, registry);
    if (!loaded) {
        std::cerr << "[PluginTests] FAIL: Failed to load dynamically linked plugin\n";
        return false;
    }

    // Verify it is in the list of loaded plugins
    auto loaded_list = pm.list_loaded_plugins();
    bool found_name = false;
    for (const auto& name : loaded_list) {
        if (name == "wipe_plugin") {
            found_name = true;
            break;
        }
    }

    if (!found_name) {
        std::cerr << "[PluginTests] FAIL: Loaded plugin list did not contain 'wipe_plugin'\n";
        return false;
    }

    // 4. Look up and resolve the custom transition
    const auto* desc = registry.resolve("plugin.test_wipe");
    if (!desc) {
        std::cerr << "[PluginTests] FAIL: Transition 'plugin.test_wipe' not found in registry after loading plugin\n";
        return false;
    }

    if (desc->id != "plugin.test_wipe") {
        std::cerr << "[PluginTests] FAIL: Transition descriptor ID mismatch\n";
        return false;
    }

    // Verify properties
    if (desc->runtime_kind != TransitionRuntimeKind::CpuPixel || !desc->capabilities.supports_cpu) {
        std::cerr << "[PluginTests] FAIL: Transition descriptor capabilities mismatch\n";
        return false;
    }

    // 5. Test executing the dynamic transition wrapper callback
    SurfaceRGBA input(100, 100);
    SurfaceRGBA to_surface(100, 100);
    input.clear(Color::black());
    to_surface.clear(Color::white());

    // Execute transition callback
    float u = 0.8f;
    float v = 0.4f;
    float t = 0.5f;
    Color result = desc->cpu_fn(u, v, t, input, &to_surface);

    // Verify color matches custom wipe formula (r = 0.8 * 0.5 = 0.4, g = 0.4 * 0.5 = 0.2, b = 0.5)
    if (std::abs(result.r - 0.4f) > 1e-4f ||
        std::abs(result.g - 0.2f) > 1e-4f ||
        std::abs(result.b - 0.5f) > 1e-4f ||
        result.a != 1.0f) {
        std::cerr << "[PluginTests] FAIL: Transition execution returned incorrect pixel color: ("
                  << result.r << ", " << result.g << ", " << result.b << ")\n";
        return false;
    }

    std::cout << "[PluginTests] Custom plugin wipe transition executed pixel-perfectly!\n";

    // 6. Cleanup loaded plugins and files
    pm.unload_all();
    std::filesystem::remove_all(out_dir);

    std::cout << "[PluginTests] All Minimal Plugin ABI unit tests passed successfully!\n";
    return true;
}
