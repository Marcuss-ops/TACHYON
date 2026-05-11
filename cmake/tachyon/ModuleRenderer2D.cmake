# TachyonRenderer2D sources and targets
# This file is consolidated from former fragmented .cmake files

set(TachyonRenderer2DBackendSources
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/backend/cpu_backend.cpp
)

set(TachyonRenderer2DColorSources
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/color/blending.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/color/blending_math.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/color/blending_parse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/color/color_management.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/color/color_management_system.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/color/lut3d.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/color/color_math.cpp
)

set(TachyonRenderer2DSurfaceSources
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/core/framebuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/core/surface_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/surface/surface_sampling.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/surface/surface_blur.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/surface/surface_composite.cpp
)

set(TachyonRenderer2DPathSources
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/path/contour/contour_processing.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/path/flattening/path_flattening.cpp
)

set(TachyonRenderer2DResourceSources
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/resource/precomp_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/resource/render_context.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/resource/texture_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/resource/resource_provider.cpp
)

set(TachyonRenderer2DCoreSources
    ${TachyonRenderer2DBackendSources}
    ${TachyonRenderer2DSurfaceSources}
    ${TachyonRenderer2DPathSources}
    ${TachyonRenderer2DResourceSources}
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/audio/audio_sampling.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/deform/mesh_deform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/math/math_utils.cpp
)

set(TachyonRenderer2DEffectsSources
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/blur/blur_effects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/color/chromatic_aberration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/color/color_effects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/effect_registry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/effect_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/core/effect_host.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/core/effect_param_access.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/core/glsl_transition_effect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/diagnostics/diagnostic_surface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/core/transitions/basic_transitions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/core/transitions/light_leaks/light_leak_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/core/transitions/light_leaks/light_leak_masks.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/core/transitions/light_leaks/light_leak_presets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/core/transitions/artistic_transitions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/core/transitions/transition_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/core/transitions/transition_downsampler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/core/transitions/transition_fast_paths.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/core/transitions/transition_fused_kernels.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/core/utility_effects.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/glitch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/distort/warp_stabilizer_effect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/effect_blur_table.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/effect_color_table.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/effect_distortion_table.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/effect_generator_table.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/effect_blur_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/effect_color_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/effect_lut_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/effect_misc_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/effect_sampling_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/effect_stylize_table.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/effect_transition_table.cpp
)

set(TachyonRenderer2DEvaluatedCompositionSources
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/effects/effect_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/matte_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/pipeline/composition_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/pipeline/layer_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/pipeline/layer_to_draw_command.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/primitives/feathered_mask_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/primitives/layer_renderer_procedural.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/primitives/layer_renderer_simple.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/primitives/mask_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/primitives/text_mesh_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/utilities/composition_utils.cpp
)

set(TachyonRenderer2DExtrasSources
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/generators/lens_flare_effect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/generators/light_leak_effect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/generators/light_leak_presets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/generators/particle_effects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/effects/utility/number_counter_effect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/primitives/media_card_mesh_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/spec/project_card.cpp
)

set(TachyonRenderer2DRasterSources
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/cpu_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/draw_primitives.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/draw_list_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/draw_list_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/mask_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/mesh_deform_apply.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/mesh_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/path/fill_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/path/path_flattener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/path/path_trimmer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/path/stroke_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/perspective_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/sdf_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/tile_grid.cpp
)

set(TachyonRenderer2DSources
    ${TachyonRenderer2DCoreSources}
    ${TachyonRenderer2DEffectsSources}
    ${TachyonRenderer2DEvaluatedCompositionSources}
    ${TachyonRenderer2DRasterSources}
    ${TachyonRenderer2DColorSources}
    ${CMAKE_CURRENT_SOURCE_DIR}/core/transition_registry.cpp
)

if(TACHYON_ENABLE_VULKAN)
    list(APPEND TachyonRenderer2DSources
        ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/backend/vulkan_backend.cpp
    )
endif()

set(TachyonRenderer2DAllSources
    ${TachyonRenderer2DSources}
    ${TachyonRenderer2DExtrasSources}
)

# --- Targets ---

add_library(TachyonRenderer2DColor STATIC ${TachyonRenderer2DColorSources})
tachyon_configure_common(TachyonRenderer2DColor)
target_link_libraries(TachyonRenderer2DColor PUBLIC TachyonColor)

add_library(TachyonRenderer2DExtras STATIC ${TachyonRenderer2DExtrasSources})
tachyon_configure_common(TachyonRenderer2DExtras)
target_link_libraries(TachyonRenderer2DExtras PUBLIC TachyonCore)

add_library(TachyonRenderer2D STATIC ${TachyonRenderer2DSources})
tachyon_configure_common(TachyonRenderer2D)

if(MSVC)
    set_source_files_properties(renderer2d/effects/core/transitions/light_leaks/light_leak_engine.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
else()
    set_source_files_properties(renderer2d/effects/core/transitions/light_leaks/light_leak_engine.cpp PROPERTIES COMPILE_FLAGS "-mavx2")
endif()
target_compile_definitions(TachyonRenderer2D PRIVATE "TACHYON_AVX2")

tachyon_link_text_deps(TachyonRenderer2D)
tachyon_link_omp(TachyonRenderer2D)
target_link_libraries(TachyonRenderer2D PUBLIC TachyonColor TachyonRenderer2DColor)
target_link_libraries(TachyonRenderer2D PUBLIC TachyonRenderer2DExtras)
target_link_libraries(TachyonRenderer2D PUBLIC TachyonText)
target_link_libraries(TachyonRenderer2D PUBLIC TachyonPresets)

