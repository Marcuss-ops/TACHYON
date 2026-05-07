#include "test_utils.h"
#include <vector>

bool run_asset_resolution_tests();
bool run_render_intent_tests();
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
namespace tachyon { bool run_tiling_tests(); }
bool run_effect_host_tests();
bool run_matte_resolver_tests();
bool run_render_session_tests();
bool run_parallax_cards_tests();
bool run_png_3d_validation_tests();
bool run_motion_blur_tests();
bool run_time_remap_tests();
bool run_frame_blend_tests();
bool run_rolling_shutter_tests();
bool run_3d_modifier_tests();

int main(int argc, char** argv) {
    using namespace tachyon::test;
    std::vector<TestCase> tests = {
        {"asset_resolution", run_asset_resolution_tests},
        {"render_intent", run_render_intent_tests},
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
        {"tiling", tachyon::run_tiling_tests},
        {"effect_host", run_effect_host_tests},
        {"matte_resolver", run_matte_resolver_tests},
        {"render_session", run_render_session_tests},
        {"parallax_cards", run_parallax_cards_tests},
        {"png_3d_validation", run_png_3d_validation_tests},
        {"motion_blur", run_motion_blur_tests},
        {"time_remap", run_time_remap_tests},
        {"frame_blend", run_frame_blend_tests},
        {"rolling_shutter", run_rolling_shutter_tests},
        {"three_d_modifier", run_3d_modifier_tests},
    };

    return run_test_suite(argc, argv, tests);
}
