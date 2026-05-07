# ---------------------------------------------------------
# 1. CORE DOMAIN: TachyonTests
# core/* (math, spec, logic)
# ---------------------------------------------------------
add_executable(TachyonTests
    unit/test_main_core.cpp
    unit/core/math_tests.cpp
    unit/core/property_tests.cpp
    unit/core/expression_tests.cpp
    unit/core/animation/property_sampler_tests.cpp
    unit/core/camera/camera_shake_tests.cpp
    unit/core/properties/bezier_interpolator_tests.cpp
    unit/timeline/camera_cuts_tests.cpp
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
)

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
