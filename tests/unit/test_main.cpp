#include <iostream>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <random>
#include <optional>
#include <iomanip>
#include <cstring>

// Global seed for random tests
uint32_t g_test_seed = 0;

uint32_t get_global_random_seed() {
    return g_test_seed;
}

namespace {

struct TestCase {
    const char* name;
    bool (*fn)();
};

std::string_view get_env_var(const char* name) {
    if (const char* value = std::getenv(name); value && *value) {
        return value;
    }
    return {};
}

std::string_view test_filter() {
    return get_env_var("TACHYON_TEST_FILTER");
}

bool matches_filter(std::string_view name) {
    const std::string_view filter = test_filter();
    if (filter.empty()) {
        return true;
    }

    std::size_t start = 0;
    while (start <= filter.size()) {
        const std::size_t end = filter.find(',', start);
        const std::string_view token = filter.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start);
        if (!token.empty() && name.find(token) != std::string_view::npos) {
            return true;
        }
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }
    return false;
}

bool run_step(const char* name, bool (*fn)(), int iteration, int total_iterations) {
    if (!matches_filter(name)) {
        std::cerr << "[SKIP] " << name << '\n';
        return true;
    }

    std::cerr << "[RUN] " << name << '\n';
    const bool ok = fn();
    
    if (ok) {
        if (total_iterations > 1) {
            std::cerr << "[OK] " << name << " (" << (iteration + 1) << "/" << total_iterations << ")\n";
        } else {
            std::cerr << "[OK] " << name << '\n';
        }
    } else {
        if (total_iterations > 1) {
            std::cerr << "[FAIL] " << name << " (iteration " << (iteration + 1) << "/" << total_iterations << ")\n";
        } else {
            std::cerr << "[FAIL] " << name << '\n';
        }
    }
    
    return ok;
}

uint32_t initialize_seed() {
    std::string_view seed_str = get_env_var("TACHYON_TEST_SEED");
    if (!seed_str.empty()) {
        try {
            return static_cast<uint32_t>(std::stoul(std::string(seed_str)));
        } catch (...) {
            // Fallback to random device if parsing fails
        }
    }
    return std::random_device{}();
}

int get_repeat_count() {
    std::string_view repeat_str = get_env_var("TACHYON_TEST_REPEAT");
    if (!repeat_str.empty()) {
        try {
            return std::stoi(std::string(repeat_str));
        } catch (...) {
            // Fallback to 1 if parsing fails
        }
    }
    return 1;
}

} // namespace

// External test declarations
bool run_scene_spec_tests();
bool run_render_job_tests();
bool run_math_tests();
bool run_asset_resolution_tests();
bool run_image_manager_tests();
bool run_image_decode_tests();
bool run_framebuffer_tests();
bool run_rasterizer_tests();
bool run_surface_tests();
bool run_draw_list_builder_tests();
bool run_blend_modes_tests();
bool run_evaluated_composition_renderer_tests();
bool run_path_rasterizer_tests();
bool run_path_rasterizer_aa_tests();
bool run_expression_vm_tests();
bool run_frame_cache_tests();
bool run_frame_cache_budget_tests();
bool run_tiling_integration_tests();
bool run_runtime_backbone_tests();
bool run_frame_executor_tests();
bool run_frame_range_tests();
bool run_frame_output_sink_tests();
bool run_tile_scheduler_tests();
bool run_render_contract_tests();
bool run_property_tests();
bool run_expression_tests();
namespace tachyon::editor { bool run_undo_manager_tests(); }
namespace tachyon::editor { bool run_autosave_manager_tests(); }
bool run_scene_evaluator_tests();
bool run_render_session_tests();
bool run_render_batch_tests();
bool run_parallax_cards_tests();
bool run_studio_library_tests();
bool run_timeline_tests();
bool run_camera_cuts_tests();
bool run_camera_shake_tests();
bool run_bezier_interpolator_tests();
bool run_track_tests();
bool run_track_binding_tests();
bool run_planar_track_tests();
bool run_camera_solver_tests();
bool run_matte_resolver_tests();
bool run_glyph_cache_tests();
bool run_effect_host_tests();
bool run_precomp_mask_tests();
bool run_golden_visual_tests();
namespace tachyon { bool run_tiling_tests(); }
bool run_optical_flow_tests();
bool run_scene3d_bridge_tests();
namespace tachyon { bool run_native_render_tests(); }
namespace tachyon { bool run_vertical_slice_tests(); }
bool run_sfx_contract_tests();
bool run_shape_contract_tests();
bool run_time_remap_tests();
bool run_frame_blend_tests();
bool run_motion_blur_tests();
bool run_rolling_shutter_tests();
bool run_audio_trim_tests();


int main(int argc, char** argv) {
    std::vector<TestCase> tests = {
        {"native_render", tachyon::run_native_render_tests},
        {"math", run_math_tests},
        {"property", run_property_tests},
        {"expression", run_expression_tests},
        {"asset_resolution", run_asset_resolution_tests},
        {"image_manager", run_image_manager_tests},
        {"image_decode", run_image_decode_tests},
        {"framebuffer", run_framebuffer_tests},
        {"rasterizer", run_rasterizer_tests},
        {"surface", run_surface_tests},
        {"draw_list_builder", run_draw_list_builder_tests},
        {"blend_modes", run_blend_modes_tests},
        {"evaluated_composition_renderer", run_evaluated_composition_renderer_tests},
        {"path_rasterizer", run_path_rasterizer_tests},
        {"path_rasterizer_aa", run_path_rasterizer_aa_tests},
        {"frame_cache", run_frame_cache_tests},
        {"frame_cache_budget", run_frame_cache_budget_tests},
        {"tiling_integration", run_tiling_integration_tests},
        {"optical_flow", run_optical_flow_tests},
        //{"runtime_backbone", run_runtime_backbone_tests},  // Quarantined - see tests/disabled/README.md
        {"frame_executor", run_frame_executor_tests},
        {"frame_range", run_frame_range_tests},
        {"frame_output_sink", run_frame_output_sink_tests},
        {"tile_scheduler", run_tile_scheduler_tests},
        {"render_contract", run_render_contract_tests},
        {"undo_manager", tachyon::editor::run_undo_manager_tests},
        {"autosave_manager", tachyon::editor::run_autosave_manager_tests},
        {"scene_evaluator", run_scene_evaluator_tests},
        {"render_session", run_render_session_tests},
        {"render_batch", run_render_batch_tests},
        {"parallax_cards", run_parallax_cards_tests},
        {"vertical_slice", tachyon::run_vertical_slice_tests},
        {"studio_library", run_studio_library_tests},
        {"timeline", run_timeline_tests},
        {"camera_cuts", run_camera_cuts_tests},
        {"camera_shake", run_camera_shake_tests},
        {"bezier_interpolator", run_bezier_interpolator_tests},
        {"track", run_track_tests},
        {"track_binding", run_track_binding_tests},
        {"planar_track", run_planar_track_tests},
        {"camera_solver", run_camera_solver_tests},
        {"matte_resolver", run_matte_resolver_tests},
        {"glyph_cache", run_glyph_cache_tests},
        // {"text", run_text_tests},  // Disabled - see tests/disabled/README.md
        {"effect_host", run_effect_host_tests},
        {"precomp_mask", run_precomp_mask_tests},
        {"golden", run_golden_visual_tests},
        {"tiling", tachyon::run_tiling_tests},
        {"scene_spec", run_scene_spec_tests},
        {"motion_blur", run_motion_blur_tests},
        // {"audio_pitch_correct", run_audio_pitch_correct_tests},  // Disabled - see tests/disabled/README.md

        {"render_job", run_render_job_tests},
        {"expression_vm", run_expression_vm_tests},
        {"sfx_contract", run_sfx_contract_tests},
        {"shape_contract", run_shape_contract_tests},
        {"time_remap", run_time_remap_tests},
        {"frame_blend", run_frame_blend_tests},
        {"rolling_shutter", run_rolling_shutter_tests},
        {"audio_trim", run_audio_trim_tests},
    };

    bool list_tests = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--list-tests") == 0 || std::strcmp(argv[i], "-l") == 0) {
            list_tests = true;
            break;
        }
    }

    if (list_tests || !get_env_var("TACHYON_LIST_TESTS").empty()) {
        std::cout << "Available tests:\n";
        size_t max_name_len = 0;
        for (const auto& test : tests) {
            max_name_len = std::max(max_name_len, std::strlen(test.name));
        }

        for (size_t i = 0; i < tests.size(); ++i) {
            std::cout << std::left << std::setw(max_name_len + 4) << tests[i].name;
            if ((i + 1) % 2 == 0) {
                std::cout << "\n";
            }
        }
        if (tests.size() % 2 != 0) {
            std::cout << "\n";
        }
        return 0;
    }

    g_test_seed = initialize_seed();
    int repeat_count = get_repeat_count();

    for (int i = 0; i < repeat_count; ++i) {
        for (const auto& test : tests) {
            if (!run_step(test.name, test.fn, i, repeat_count)) {
                std::cerr << test.name << " tests failed\n";
                return 1;
            }
        }
    }

    if (repeat_count > 1) {
        std::cout << "All tests passed! (" << repeat_count << " iterations, seed=" << g_test_seed << ")\n";
    } else {
        std::cout << "All tests passed! (seed=" << g_test_seed << ")\n";
    }

    return 0;
}
