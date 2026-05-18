# Tachyon Core Module

set(TachyonCoreSources
    animation_primitives.cpp
    animation/property_interpolation.cpp
    animation/property_sampler.cpp
    math/deterministic_random.cpp
    properties/bezier_interpolator.cpp
    shapes/shape_modifiers.cpp
    expressions/context_builder.cpp
    expressions/expression_engine.cpp
    expressions/expression_vm.cpp
    io/path_resolution.cpp
    timeline/timeline_plan.cpp
    spec/schema/common/common_spec.cpp
    transition/transition_descriptor.cpp
    transition/transition_resolver.cpp
    transition/transition_effect_resolver.cpp
    transition/transition_manifest.cpp
    transition/transition_simd_kernels.cpp
    model/tachyon.cpp
    audio/audio_utils.cpp
    audio/audio_filter_utils.cpp
    assets/asset_collector.cpp
    assets/asset_resolution.cpp
    presets/sfx/sfx_builders.cpp
    presets/sfx/sfx_registry.cpp
    presets/sfx/sfx_registry_table.cpp
    profiles/execution_profile.cpp
    render_telemetry.cpp
    # Infrastructure adapters
    diag/log.cpp
    compression/compress.cpp
    cli_bridge.cpp
)


# We keep the target definition here as recommended.
add_library(TachyonCore STATIC ${TachyonCoreSources})
tachyon_configure_common(TachyonCore)
target_compile_definitions(TachyonCore PUBLIC TACHYON_BUILD_CONFIG_NAME="$<CONFIG>")
tachyon_link_omp(TachyonCore)
tachyon_link_absl(TachyonCore)
tachyon_link_infra(TachyonCore)

if(TACHYON_ENABLE_TRACY)
    target_link_libraries(TachyonCore PUBLIC TracyClient)
    target_compile_definitions(TachyonCore PUBLIC TACHYON_ENABLE_TRACY)
endif()


# Clipper2 integration
target_include_directories(TachyonCore PUBLIC $<BUILD_INTERFACE:${clipper2_SOURCE_DIR}/CPP/Clipper2Lib/include>)
target_sources(TachyonCore PRIVATE
    ${clipper2_SOURCE_DIR}/CPP/Clipper2Lib/src/clipper.engine.cpp
    ${clipper2_SOURCE_DIR}/CPP/Clipper2Lib/src/clipper.offset.cpp
    ${clipper2_SOURCE_DIR}/CPP/Clipper2Lib/src/clipper.rectclip.cpp
)

# Additional Core-related modules
add_subdirectory(platform)
