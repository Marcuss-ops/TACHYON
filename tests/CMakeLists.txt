add_executable(TachyonTests
    unit/test_main.cpp
    unit/core/math_tests.cpp
    unit/core/property_tests.cpp
    unit/core/expression_tests.cpp
    unit/media/asset_resolution_tests.cpp
    unit/media/image_manager_tests.cpp
    unit/media/image_decode_tests.cpp
    unit/renderer2d/framebuffer_tests.cpp
    unit/renderer2d/rasterizer_tests.cpp
    unit/renderer2d/surface_tests.cpp
    unit/renderer2d/draw_list_builder_tests.cpp
    unit/renderer2d/blend_modes_tests.cpp
    unit/renderer2d/evaluated_composition_renderer_tests.cpp
    unit/renderer2d/path_rasterizer_tests.cpp
    unit/renderer2d/path_rasterizer_aa_tests.cpp
    unit/renderer2d/effect_host_tests.cpp
    unit/runtime/frame_cache_tests.cpp
    unit/runtime/frame_executor_tests.cpp
    unit/runtime/frame_output_sink_tests.cpp
    unit/runtime/tile_scheduler_tests.cpp
    unit/runtime/render_contract_tests.cpp
    unit/core/scene/evaluator_tests.cpp
    unit/core/scene/precomp_mask_tests.cpp
    unit/timeline/time_tests.cpp
    unit/core/spec/scene_spec_tests.cpp
    unit/text/glyph_cache_tests.cpp
    unit/text/text_tests.cpp
    integration/render_session_tests.cpp
    integration/render_batch_tests.cpp
    integration/parallax_cards_tests.cpp
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

if(MSVC)
    target_compile_options(TachyonTests PRIVATE /FS)
endif()
