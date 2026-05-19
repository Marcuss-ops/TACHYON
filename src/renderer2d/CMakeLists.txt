# Tachyon Renderer2D Module

set(TachyonRenderer2DBackendSources
    backend/cpu_backend.cpp
)

set(TachyonRenderer2DColorSources
    color/blending.cpp
    color/blending_math.cpp
    color/blending_parse.cpp
    color/color_management.cpp
    color/color_management_system.cpp
    color/color_math.cpp
)

set(TachyonRenderer2DSurfaceSources
    core/framebuffer.cpp
    surface/surface_sampling.cpp
    surface/surface_blur.cpp
    surface/surface_composite.cpp
    surface/surface_composite_avx2.cpp
)

if(TACHYON_ENABLE_HIGHWAY)
    list(APPEND TachyonRenderer2DSurfaceSources surface/surface_ops_highway.cpp)
endif()

set(TachyonRenderer2DPathSources
    path/contour/contour_processing.cpp
    path/flattening/path_flattening.cpp
)

set(TachyonRenderer2DResourceSources
    resource/precomp_cache.cpp
    resource/texture_resolver.cpp
    resource/resource_provider.cpp
)

set(TachyonRenderer2DCoreSources
    ${TachyonRenderer2DBackendSources}
    ${TachyonRenderer2DSurfaceSources}
    ${TachyonRenderer2DPathSources}
    ${TachyonRenderer2DResourceSources}
    audio/audio_sampling.cpp
    geometry/dirty_region.cpp
)

set(TachyonRenderer2DEffectsSources
    effects/blur/blur_effects.cpp
    effects/color/chromatic_aberration.cpp
    effects/color/color_effects.cpp
    effects/effect_registry.cpp
    effects/effect_resolver.cpp
    effects/effect_validation.cpp
    effects/core/effect_host.cpp
    effects/core/effect_param_access.cpp
    effects/core/glsl_transition_effect.cpp
    effects/diagnostics/diagnostic_surface.cpp
    effects/core/transitions/basic_transitions.cpp
    effects/core/transitions/light_leaks/light_leak_engine.cpp
    effects/core/transitions/light_leaks/light_leak_masks.cpp
    effects/core/transitions/light_leaks/light_leak_presets.cpp
    effects/core/transitions/artistic_transitions.cpp
    effects/core/transitions/transition_utils.cpp
    effects/core/transitions/transition_downsampler.cpp
    effects/core/transitions/transition_fast_paths.cpp
    effects/core/transitions/transition_fused_kernels.cpp
    effects/core/transitions/transition_apply.cpp
    effects/core/utility_effects.cpp
    effects/glitch.cpp
    effects/distort/warp_stabilizer_effect.cpp
    effects/effect_blur_table.cpp
    effects/effect_color_table.cpp
    effects/effect_distortion_table.cpp
    effects/effect_generator_table.cpp
    effects/effect_blur_utils.cpp
    effects/effect_color_utils.cpp
    effects/effect_lut_utils.cpp
    effects/effect_sampling_utils.cpp
    effects/effect_stylize_table.cpp
    effects/effect_transition_table.cpp
)

set(TachyonRenderer2DEvaluatedCompositionSources
    evaluated_composition/effects/effect_renderer.cpp
    evaluated_composition/matte_resolver.cpp
    evaluated_composition/rendering/pipeline/composition_renderer.cpp
    evaluated_composition/rendering/pipeline/layer_renderer.cpp
    evaluated_composition/rendering/pipeline/layer_to_draw_command.cpp
    evaluated_composition/rendering/pipeline/pipeline_helpers.cpp
    evaluated_composition/rendering/primitives/feathered_mask_renderer.cpp
    evaluated_composition/rendering/primitives/layer_renderer_procedural.cpp
    evaluated_composition/rendering/primitives/layer_renderer_simple.cpp
    evaluated_composition/rendering/primitives/mask_renderer.cpp
    evaluated_composition/utilities/composition_utils.cpp
)

set(TachyonRenderer2DExtrasSources
    effects/generators/lens_flare_effect.cpp
    effects/generators/light_leak_effect.cpp
    effects/generators/light_leak_presets.cpp
    effects/generators/particle_effects.cpp
    effects/utility/number_counter_effect.cpp
)

set(TachyonRenderer2DRasterSources
    raster/cpu_rasterizer.cpp
    raster/draw_primitives.cpp
    raster/draw_list_builder.cpp
    raster/draw_list_rasterizer.cpp
    raster/mask_rasterizer.cpp
    raster/path/fill_rasterizer.cpp
    raster/path/path_flattener.cpp
    raster/path/path_trimmer.cpp
    raster/path/stroke_rasterizer.cpp
    raster/sdf_rasterizer.cpp
    raster/perspective_rasterizer.cpp
    raster/tile_grid.cpp
)

set(TachyonRenderer2DSources
    ${TachyonRenderer2DCoreSources}
    ${TachyonRenderer2DEffectsSources}
    ${TachyonRenderer2DEvaluatedCompositionSources}
    ${TachyonRenderer2DRasterSources}
    ${TachyonRenderer2DColorSources}
    ../core/transition_registry.cpp
    transition_registry_factory.cpp
)

if(TACHYON_ENABLE_VULKAN)
    list(APPEND TachyonRenderer2DSources backend/vulkan_backend.cpp)
endif()

# Targets
add_library(TachyonRenderer2DColor STATIC ${TachyonRenderer2DColorSources})
tachyon_configure_common(TachyonRenderer2DColor)
target_link_libraries(TachyonRenderer2DColor PUBLIC TachyonColor)

add_library(TachyonRenderer2DExtras STATIC ${TachyonRenderer2DExtrasSources})
tachyon_configure_common(TachyonRenderer2DExtras)
target_link_libraries(TachyonRenderer2DExtras PUBLIC TachyonCore)

add_library(TachyonRenderer2D STATIC ${TachyonRenderer2DSources})
tachyon_configure_common(TachyonRenderer2D)

if(MSVC)
    target_compile_options(TachyonRenderer2D PRIVATE /bigobj)
    set_source_files_properties(effects/core/transitions/light_leaks/light_leak_engine.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
    set_source_files_properties(surface/surface_composite_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
else()
    set_source_files_properties(effects/core/transitions/light_leaks/light_leak_engine.cpp PROPERTIES COMPILE_FLAGS "-mavx2")
    set_source_files_properties(surface/surface_composite_avx2.cpp PROPERTIES COMPILE_FLAGS "-mavx2 -mfma")
endif()
target_compile_definitions(TachyonRenderer2D PRIVATE "TACHYON_AVX2")

tachyon_link_text_deps(TachyonRenderer2D)
tachyon_link_omp(TachyonRenderer2D)
target_link_libraries(TachyonRenderer2D PUBLIC 
    TachyonCore 
    TachyonColor 
    TachyonRenderer2DColor
    TachyonRenderer2DExtras
    TachyonPresets
)



if(TACHYON_ENABLE_HIGHWAY)
    if(TARGET hwy)
        target_link_libraries(TachyonRenderer2D PUBLIC hwy)
    elseif(TARGET hwy::hwy)
        target_link_libraries(TachyonRenderer2D PUBLIC hwy::hwy)
    endif()
    target_compile_definitions(TachyonRenderer2D PUBLIC TACHYON_ENABLE_HIGHWAY)
endif()

if(TARGET TachyonBackendSimd)
    target_link_libraries(TachyonRenderer2D PUBLIC TachyonBackendSimd)
endif()
