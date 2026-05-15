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

)

if(TACHYON_ENABLE_SIMDJSON)
    list(APPEND TachyonCoreSources json/json_parser_simdjson.cpp)
endif()

if(TACHYON_ENABLE_HIGHWAY)
    list(APPEND TachyonCoreSources
        transition/transition_simd_kernels_highway.cpp
        simd/conversion_highway.cpp
    )
endif()

# We keep the target definition here as recommended.
add_library(TachyonCore STATIC ${TachyonCoreSources})
tachyon_configure_common(TachyonCore)
tachyon_link_omp(TachyonCore)
tachyon_link_absl(TachyonCore)

if(TACHYON_ENABLE_TRACY)
    target_link_libraries(TachyonCore PUBLIC TracyClient)
    target_compile_definitions(TachyonCore PUBLIC TACHYON_ENABLE_TRACY)
endif()

if(TACHYON_ENABLE_SIMDJSON)
    target_compile_definitions(TachyonCore PUBLIC TACHYON_ENABLE_SIMDJSON)
    if(TARGET simdjson::simdjson)
        target_link_libraries(TachyonCore PUBLIC simdjson::simdjson)
    elseif(TARGET simdjson)
        target_link_libraries(TachyonCore PUBLIC simdjson)
    endif()
endif()

if(TACHYON_ENABLE_HIGHWAY)
    if(TARGET hwy)
        target_link_libraries(TachyonCore PUBLIC hwy)
    elseif(TARGET hwy::hwy)
        target_link_libraries(TachyonCore PUBLIC hwy::hwy)
    endif()
    target_compile_definitions(TachyonCore PUBLIC TACHYON_ENABLE_HIGHWAY)
endif()

# Clipper2 integration
target_include_directories(TachyonCore PUBLIC $<BUILD_INTERFACE:${clipper2_SOURCE_DIR}/CPP/Clipper2Lib/include>)
target_sources(TachyonCore PRIVATE
    ${clipper2_SOURCE_DIR}/CPP/Clipper2Lib/src/clipper.engine.cpp
    ${clipper2_SOURCE_DIR}/CPP/Clipper2Lib/src/clipper.offset.cpp
    ${clipper2_SOURCE_DIR}/CPP/Clipper2Lib/src/clipper.rectclip.cpp
)

if(TACHYON_ENABLE_MANUAL_AVX2)
    target_sources(TachyonCore PRIVATE transition/transition_simd_kernels_avx2.cpp)
    if(MSVC)
        set_source_files_properties(transition/transition_simd_kernels_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
    else()
        set_source_files_properties(transition/transition_simd_kernels_avx2.cpp PROPERTIES COMPILE_FLAGS "-mavx2")
    endif()
    target_compile_definitions(TachyonCore PRIVATE "TACHYON_AVX2")
endif()

# Additional Core-related modules
add_subdirectory(platform)
add_subdirectory(c_api)
