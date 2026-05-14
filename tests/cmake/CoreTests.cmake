# ---------------------------------------------------------
# 1. CORE DOMAIN: TachyonTests
# core/* (math, spec, logic)
# ---------------------------------------------------------
add_executable(TachyonTests
    unit/mains/test_main_core.cpp
    unit/core/math_tests.cpp
    unit/core/property_tests.cpp
    unit/core/expression_tests.cpp
    unit/core/animation/property_sampler_tests.cpp
    unit/core/properties/bezier_interpolator_tests.cpp
    unit/core/transition_simd_kernels_tests.cpp
    unit/core/json_parser_tests.cpp
    unit/diagnostics/trace_test.cpp
    unit/core/scene/builder_preset_tests.cpp
    unit/color/blend_kernel_tests.cpp
)

target_compile_definitions(TachyonTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonTests
    PRIVATE
        TachyonTestUtils
        TachyonRuntimeEngine
        TachyonDiagnostics
        TachyonPresets
)
tachyon_link_mimalloc(TachyonTests)

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(TachyonTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

if(MSVC)
    target_compile_options(TachyonTests PRIVATE /FS)
endif()

add_executable(TachyonColorTests
    unit/color/color_tests.cpp
    unit/color/blend_kernel_tests.cpp
)

target_link_libraries(TachyonColorTests
    PRIVATE
        TachyonTestUtils
        TachyonColor
)

if(MSVC)
    target_compile_options(TachyonColorTests PRIVATE /FS)
endif()

add_test(
    NAME TachyonTests
    COMMAND TachyonTests
)
tachyon_set_test_labels(TachyonTests core)

add_test(
    NAME TachyonColorTests
    COMMAND TachyonColorTests
)
tachyon_set_test_labels(TachyonColorTests core)

# PR 1: Strict Registry Tests
add_executable(transition_registry_duplicate_policy_tests unit/core/transition/transition_registry_duplicate_policy_tests.cpp)
target_link_libraries(transition_registry_duplicate_policy_tests PRIVATE TachyonCore TachyonTestUtils)
add_test(NAME transition_registry_duplicate_policy COMMAND transition_registry_duplicate_policy_tests)

# PR 3: Transition Apply Tests
add_executable(transition_apply_equivalence_tests unit/core/transition/transition_apply_equivalence_tests.cpp)
target_link_libraries(transition_apply_equivalence_tests PRIVATE 
    TachyonCore 
    TachyonRuntime
    TachyonPresets
    TachyonTestUtils
    TachyonRenderer2D
    TachyonPlatform
)
add_test(NAME transition_apply_equivalence COMMAND transition_apply_equivalence_tests)
tachyon_set_test_labels(transition_apply_equivalence core)

# PR 4: JIT and Smoke Tests
add_executable(authoring_service_cache_tests unit/core/spec/authoring_service_cache_tests.cpp)
target_link_libraries(authoring_service_cache_tests PRIVATE 
    TachyonCore 
    TachyonScene
    TachyonAudio
    TachyonColor
    TachyonPlatform
    TachyonTestUtils
)
add_test(NAME authoring_service_cache COMMAND authoring_service_cache_tests)
tachyon_set_test_labels(authoring_service_cache core jit)

add_executable(cpp_scene_loader_host_api_tests unit/core/spec/cpp_scene_loader_host_api_tests.cpp)
target_link_libraries(cpp_scene_loader_host_api_tests PRIVATE 
    TachyonCore 
    TachyonScene
    TachyonTestUtils
)
add_test(NAME cpp_scene_loader_host_api COMMAND cpp_scene_loader_host_api_tests)
tachyon_set_test_labels(cpp_scene_loader_host_api core jit)

add_executable(transition_clip_pair_smoke_tests smoke/transitions/transition_clip_pair_smoke_tests.cpp)
target_link_libraries(transition_clip_pair_smoke_tests PRIVATE 
    TachyonCore 
    TachyonRuntime
    TachyonPresets
    TachyonTestUtils
    TachyonRenderer2D
    TachyonPlatform
)
add_test(NAME transition_clip_pair_smoke COMMAND transition_clip_pair_smoke_tests)
tachyon_set_test_labels(transition_clip_pair_smoke smoke transitions)
tachyon_set_test_labels(transition_registry_duplicate_policy core)
