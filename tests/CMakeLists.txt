add_executable(TachyonTests
    unit/test_main.cpp
    unit/core/math_tests.cpp
    unit/core/property_tests.cpp
    unit/core/expression_tests.cpp
    unit/media/asset_resolution_tests.cpp
    unit/render/framebuffer_tests.cpp
    unit/render/rasterizer_tests.cpp
    unit/renderer2d/surface_tests.cpp
    unit/renderer2d/draw_list_builder_tests.cpp
    unit/runtime/frame_cache_tests.cpp
    unit/runtime/frame_executor_tests.cpp
    unit/scene/evaluator_tests.cpp
    unit/timeline/time_tests.cpp
    unit/spec/scene_spec_tests.cpp
    unit/text/text_tests.cpp
    integration/render_session_tests.cpp
)

set(TACHYON_TESTS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
string(REPLACE "\\" "/" TACHYON_TESTS_SOURCE_DIR "${TACHYON_TESTS_SOURCE_DIR}")

target_compile_definitions(TachyonTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonTests
    PRIVATE
        TachyonCore
)
