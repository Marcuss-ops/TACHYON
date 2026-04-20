#include <iostream>

namespace {

bool run_step(const char* name, bool (*fn)()) {
    std::cerr << "[RUN] " << name << '\n';
    const bool ok = fn();
    std::cerr << "[OK] " << name << '\n';
    return ok;
}

} // namespace

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
bool run_frame_cache_tests();
bool run_frame_executor_tests();
bool run_frame_output_sink_tests();
bool run_tile_scheduler_tests();
bool run_render_contract_tests();
bool run_property_tests();
bool run_expression_tests();
bool run_scene_evaluator_tests();
bool run_render_session_tests();
bool run_render_batch_tests();
bool run_parallax_cards_tests();
bool run_timeline_tests();
bool run_glyph_cache_tests();
bool run_text_tests();
bool run_effect_host_tests();
bool run_precomp_mask_tests();

int main() {
    if (!run_step("math", run_math_tests)) {
        std::cerr << "math tests failed\n";
        return 1;
    }
    if (!run_step("property", run_property_tests)) {
        std::cerr << "property tests failed\n";
        return 1;
    }
    if (!run_step("expression", run_expression_tests)) {
        std::cerr << "expression tests failed\n";
        return 1;
    }
    if (!run_step("asset_resolution", run_asset_resolution_tests)) {
        std::cerr << "asset resolution tests failed\n";
        return 1;
    }
    if (!run_step("image_manager", run_image_manager_tests)) {
        std::cerr << "image manager tests failed\n";
        return 1;
    }
    if (!run_step("image_decode", run_image_decode_tests)) {
        std::cerr << "image decode tests failed\n";
        return 1;
    }
    if (!run_step("framebuffer", run_framebuffer_tests)) {
        std::cerr << "framebuffer tests failed\n";
        return 1;
    }
    if (!run_step("rasterizer", run_rasterizer_tests)) {
        std::cerr << "rasterizer tests failed\n";
        return 1;
    }
    if (!run_step("surface", run_surface_tests)) {
        std::cerr << "surface tests failed\n";
        return 1;
    }
    if (!run_step("draw_list_builder", run_draw_list_builder_tests)) {
        std::cerr << "draw list builder tests failed\n";
        return 1;
    }
    if (!run_step("blend_modes", run_blend_modes_tests)) {
        std::cerr << "blend modes tests failed\n";
        return 1;
    }
    if (!run_step("evaluated_composition_renderer", run_evaluated_composition_renderer_tests)) {
        std::cerr << "evaluated composition renderer tests failed\n";
        return 1;
    }
    if (!run_step("path_rasterizer", run_path_rasterizer_tests)) {
        std::cerr << "path rasterizer tests failed\n";
        return 1;
    }
    if (!run_step("path_rasterizer_aa", run_path_rasterizer_aa_tests)) {
        std::cerr << "path rasterizer aa tests failed\n";
        return 1;
    }
    if (!run_step("frame_cache", run_frame_cache_tests)) {
        std::cerr << "frame cache tests failed\n";
        return 1;
    }
    if (!run_step("frame_executor", run_frame_executor_tests)) {
        std::cerr << "frame executor tests failed\n";
        return 1;
    }
    if (!run_step("frame_output_sink", run_frame_output_sink_tests)) {
        std::cerr << "frame output sink tests failed\n";
        return 1;
    }
    if (!run_step("tile_scheduler", run_tile_scheduler_tests)) {
        std::cerr << "tile scheduler tests failed\n";
        return 1;
    }
    if (!run_step("render_contract", run_render_contract_tests)) {
        std::cerr << "render contract tests failed\n";
        return 1;
    }
    if (!run_step("scene_evaluator", run_scene_evaluator_tests)) {
        std::cerr << "scene evaluator tests failed\n";
        return 1;
    }
    if (!run_step("render_session", run_render_session_tests)) {
        std::cerr << "render session tests failed\n";
        return 1;
    }
    if (!run_step("render_batch", run_render_batch_tests)) {
        std::cerr << "render batch tests failed\n";
        return 1;
    }
    if (!run_step("parallax_cards", run_parallax_cards_tests)) {
        std::cerr << "camera card tests failed\n";
        return 1;
    }
    if (!run_step("timeline", run_timeline_tests)) {
        std::cerr << "timeline tests failed\n";
        return 1;
    }
    if (!run_step("glyph_cache", run_glyph_cache_tests)) {
        std::cerr << "glyph cache tests failed\n";
        return 1;
    }
    if (!run_step("text", run_text_tests)) {
        std::cerr << "text tests failed\n";
        return 1;
    }
    if (!run_step("effect_host", run_effect_host_tests)) {
        std::cerr << "effect host tests failed\n";
        return 1;
    }
    if (!run_step("precomp_mask", run_precomp_mask_tests)) {
        std::cerr << "precomp and mask tests failed\n";
        return 1;
    }
    if (!run_step("scene_spec", run_scene_spec_tests)) {
        std::cerr << "scene spec tests failed\n";
        return 1;
    }
    if (!run_step("render_job", run_render_job_tests)) {
        std::cerr << "render job tests failed\n";
        return 1;
    }
    std::cout << "all tests passed\n";
    return 0;
}
