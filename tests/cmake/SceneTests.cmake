# ---------------------------------------------------------
# 2. SCENE DOMAIN: TachyonSceneTests
# scene/* (eval, inspector)
# ---------------------------------------------------------
add_executable(TachyonSceneTests
    unit/test_main_scene.cpp
    unit/core/scene/evaluator_tests.cpp
    unit/timeline/time_tests.cpp
    unit/core/analysis/scene_inspector_tests.cpp
    unit/core/analysis/motion_map_tests.cpp
    unit/core/camera/default_camera_tests.cpp
)

target_compile_definitions(TachyonSceneTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonSceneTests
    PRIVATE
        TachyonTestUtils
        TachyonRuntimeEngine
        TachyonDiagnostics
)

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(TachyonSceneTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

if(MSVC)
    target_compile_options(TachyonSceneTests PRIVATE /FS)
endif()

add_test(
    NAME TachyonSceneTests
    COMMAND TachyonSceneTests
)
tachyon_set_test_labels(TachyonSceneTests core)
