# ---------------------------------------------------------
# 4. TOOLS/CLI DOMAIN: TachyonCliTests
# integration/*, spec/cli
# ---------------------------------------------------------
add_executable(TachyonCliTests
    unit/test_main_cli.cpp
    unit/core/spec/cli_tests.cpp
    unit/core/spec/scene_spec_tests.cpp
    integration/vertical_slice_tests.cpp
)

target_compile_definitions(TachyonCliTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonCliTests
    PRIVATE
        TachyonTestUtils
        TachyonTools
)

if(TACHYON_ENABLE_TRACKER)
    target_link_libraries(TachyonCliTests PRIVATE TachyonTracker)
endif()

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(TachyonCliTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

if(MSVC)
    target_compile_options(TachyonCliTests PRIVATE /FS)
endif()

add_executable(TachyonEditorTests
    unit/test_main_editor.cpp
    unit/editor/undo_manager_tests.cpp
    unit/editor/autosave_manager_tests.cpp
)

target_compile_definitions(TachyonEditorTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonEditorTests
    PRIVATE
        TachyonTestUtils
        TachyonEditor
        TachyonCore
        TachyonScene
)

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(TachyonEditorTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

if(MSVC)
    target_compile_options(TachyonEditorTests PRIVATE /FS)
endif()

add_test(
    NAME TachyonCliTests
    COMMAND TachyonCliTests
)
tachyon_set_test_labels(TachyonCliTests core)

add_test(
    NAME TachyonEditorTests
    COMMAND TachyonEditorTests
)
tachyon_set_test_labels(TachyonEditorTests core)
