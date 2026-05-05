#include "test_utils.h"
#include <vector>

// External test declarations
bool run_scene_spec_tests();
bool run_scene_inspector_tests();
bool run_motion_map_tests();
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
bool run_frame_adapter_tests();
bool run_frame_output_sink_tests();
bool run_tile_scheduler_tests();
bool run_property_tests();
bool run_expression_tests();
namespace tachyon::editor { bool run_undo_manager_tests(); }
namespace tachyon::editor { bool run_autosave_manager_tests(); }
bool run_scene_evaluator_tests();
bool run_render_session_tests();
bool run_parallax_cards_tests();
bool run_png_3d_validation_tests();
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
bool run_scene3d_smoke_tests();
bool run_3d_modifier_tests();
void run_default_camera_tests();
void run_parallax_tests();
void run_look_at_tests();
bool run_default_camera_tests_adapter() { run_default_camera_tests(); return true; }
namespace tachyon::profiling { bool run_profiler_tests(); }

int main(int argc, char** argv) {
    using namespace tachyon::test;
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
        {"frame_executor", run_frame_executor_tests},
        {"frame_range", run_frame_range_tests},
        {"frame_adapter", run_frame_adapter_tests},
        {"frame_output_sink", run_frame_output_sink_tests},
        {"tile_scheduler", run_tile_scheduler_tests},
        {"undo_manager", tachyon::editor::run_undo_manager_tests},
        {"autosave_manager", tachyon::editor::run_autosave_manager_tests},
        {"scene_evaluator", run_scene_evaluator_tests},
        {"render_session", run_render_session_tests},
        {"parallax_cards", run_parallax_cards_tests},
        {"png_3d_validation", run_png_3d_validation_tests},
        {"vertical_slice", tachyon::run_vertical_slice_tests},
        {"timeline", run_timeline_tests},
        {"camera_cuts", run_camera_cuts_tests},
        {"camera_shake", run_camera_shake_tests},
        {"bezier_interpolator", run_bezier_interpolator_tests},
        {"track", run_track_tests},
        {"track_binding", run_track_binding_tests},
        {"planar_track", run_planar_track_tests},
        {"camera_solver", run_camera_solver_tests},
        {"matte_resolver", run_matte_resolver_tests},
        {"effect_host", run_effect_host_tests},
        {"precomp_mask", run_precomp_mask_tests},
        {"tiling", tachyon::run_tiling_tests},
        {"scene_spec", run_scene_spec_tests},
        {"scene_inspector", run_scene_inspector_tests},
        {"motion_map", run_motion_map_tests},
        {"motion_blur", run_motion_blur_tests},
        {"render_job", run_render_job_tests},
        {"expression_vm", run_expression_vm_tests},
        {"time_remap", run_time_remap_tests},
        {"frame_blend", run_frame_blend_tests},
        {"rolling_shutter", run_rolling_shutter_tests},
        {"scene3d_smoke", run_scene3d_smoke_tests},
        {"three_d_modifier", run_3d_modifier_tests},
        {"default_camera", run_default_camera_tests_adapter},
        {"profiling", tachyon::profiling::run_profiler_tests},
        {"parallax", []() { run_parallax_tests(); return true; }},
        {"lookat", []() { run_look_at_tests(); return true; }},
    };

    return run_test_suite(argc, argv, tests);
}
