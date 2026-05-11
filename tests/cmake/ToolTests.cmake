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

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(TachyonCliTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

if(MSVC)
    target_compile_options(TachyonCliTests PRIVATE /FS)
endif()

add_test(
    NAME TachyonCliTests
    COMMAND TachyonCliTests
)
tachyon_set_test_labels(TachyonCliTests core)
