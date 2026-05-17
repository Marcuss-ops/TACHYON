#include "test_utils.h"
#include <vector>

bool run_asset_resolution_tests();
bool run_image_manager_tests();
bool run_image_decode_tests();
bool run_media_manager_cache_tests();
bool run_framebuffer_tests();
bool run_rasterizer_tests();
bool run_surface_tests();
bool run_draw_list_builder_tests();
bool run_blend_modes_tests();
bool run_evaluated_composition_renderer_tests();
bool run_path_rasterizer_tests();
bool run_path_rasterizer_aa_tests();
namespace tachyon { bool run_tiling_tests(); }
bool run_effect_host_tests();
bool run_matte_resolver_tests();
bool run_transition_fast_paths_tests();
bool run_light_leak_transitions_tests();

namespace tachyon::test {
bool run_golden_smoke_test();
}

int main(int argc, char** argv) {
    using namespace tachyon::test;
    std::vector<TestCase> tests = {
        {"asset_resolution", run_asset_resolution_tests},
        {"image_manager", run_image_manager_tests},
        {"image_decode", run_image_decode_tests},
        {"media_manager_cache", run_media_manager_cache_tests},
        {"framebuffer", run_framebuffer_tests},
        {"rasterizer", run_rasterizer_tests},
        {"surface", run_surface_tests},
        {"draw_list_builder", run_draw_list_builder_tests},
        {"blend_modes", run_blend_modes_tests},
        {"transition_fast_paths", run_transition_fast_paths_tests},
        {"light_leak_transitions", run_light_leak_transitions_tests},
        {"golden_smoke", tachyon::test::run_golden_smoke_test},
        {"evaluated_composition_renderer", run_evaluated_composition_renderer_tests},
        {"path_rasterizer", run_path_rasterizer_tests},
        {"path_rasterizer_aa", run_path_rasterizer_aa_tests},
        {"tiling", tachyon::run_tiling_tests},
        {"effect_host", run_effect_host_tests},
        {"matte_resolver", run_matte_resolver_tests},
    };

    return run_test_suite(argc, argv, tests);
}
