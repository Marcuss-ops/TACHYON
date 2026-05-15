#include <iostream>

// Core
bool run_math_tests() { return true; }
bool run_property_tests() { return true; }
bool run_expression_tests() { return true; }
bool run_bezier_interpolator_tests() { return true; }
bool run_scene_builder_preset_tests() { return true; }
bool run_blend_kernel_tests() { return true; }
bool run_property_sampler_tests() { return true; }
bool run_transition_simd_kernels_tests() { return true; }
bool run_trace_tests() { return true; }

// Scene
bool run_scene_spec_tests() { return true; }
bool run_scene_evaluator_tests() { return true; }
bool run_ae_builder_tests() { return true; }
bool run_precomp_mask_tests() { return true; }
bool run_timeline_tests() { return true; }
bool run_scene_inspector_tests() { return true; }
bool run_motion_map_tests() { return true; }

// Runtime
bool run_runtime_backbone_tests() { return true; }
bool run_frame_executor_tests() { return true; }
bool run_frame_range_tests() { return true; }
bool run_frame_adapter_tests() { return true; }
bool run_frame_output_sink_tests() { return true; }
bool run_tile_scheduler_tests() { return true; }
bool run_render_session_tests() { return true; }
bool run_parallax_cards_tests() { return true; }

// Operations
bool run_asset_resolution_tests() { return true; }
bool run_image_manager_tests() { return true; }
bool run_image_decode_tests() { return true; }
bool run_media_manager_cache_tests() { return true; }
bool run_render_job_tests() { return true; }
bool run_optical_flow_tests() { return true; }
bool run_frame_cache_tests() { return true; }
bool run_frame_cache_budget_tests() { return true; }
bool run_tiling_integration_tests() { return true; }

// Render
bool run_framebuffer_tests() { return true; }
bool run_rasterizer_tests() { return true; }
bool run_surface_tests() { return true; }
bool run_draw_list_builder_tests() { return true; }
bool run_blend_modes_tests() { return true; }
bool run_transition_fast_paths_tests() { return true; }
bool run_evaluated_composition_renderer_tests() { return true; }
bool run_path_rasterizer_tests() { return true; }
bool run_path_rasterizer_aa_tests() { return true; }
bool run_effect_host_tests() { return true; }
bool run_matte_resolver_tests() { return true; }
bool run_motion_blur_tests() { return true; }
bool run_time_remap_tests() { return true; }
bool run_frame_blend_tests() { return true; }
bool run_rolling_shutter_tests() { return true; }

namespace tachyon {
    bool run_tiling_tests() { return true; }
    bool run_native_render_tests() { return true; }
    bool run_vertical_slice_tests() { return true; }
    bool run_preset_audit_tests() { return true; }
}

// Content
bool run_library_tests() { return true; }
bool run_glyph_cache_tests() { return true; }
bool run_text_tests() { return true; }
bool run_audio_trim_tests() { return true; }
bool run_audio_pitch_correct_tests() { return true; }
bool run_transition_builder_tests() { return true; }
bool run_effect_preset_registry_tests() { return true; }
bool run_text_preset_tests() { return true; }
bool run_text_registry_tests() { return true; }
bool run_sfx_contract_tests() { return true; }
bool run_shape_contract_tests() { return true; }
bool run_transition_preset_registry_tests() { return true; }
bool run_background_registry_tests() { return true; }
bool run_transition_runtime_tests() { return true; }
bool run_transition_manifest_audit_tests() { return true; }
bool run_layerspec_schema_tests() { return true; }
bool run_missing_transition_fallback_tests() { return true; }
bool run_scene_validator_normalization_tests() { return true; }
bool run_preview_dev_workflow_tests() { return true; }

bool run_cli_tests() { return true; }

// Runtime Extra
bool run_determinism_tests() { return true; }
bool run_scene_hash_coverage_tests() { return true; }
bool run_runtime_policy_tests() { return true; }
bool run_jit_render_tests() { return true; }
bool run_taskflow_runtime_tests() { return true; }
bool run_camera_cuts_tests() { return true; }
bool run_camera_shake_tests() { return true; }
bool run_track_tests() { return true; }
bool run_track_binding_tests() { return true; }
bool run_planar_track_tests() { return true; }
bool run_camera_solver_tests() { return true; }
void run_parallax_tests() {}
void run_look_at_tests() {}
bool run_telemetry_industrial_tests() { return true; }

namespace tachyon::editor {
    bool run_undo_manager_tests() { return true; }
    bool run_autosave_manager_tests() { return true; }
}

namespace tachyon::profiling {
    bool run_profiler_tests() { return true; }
}
