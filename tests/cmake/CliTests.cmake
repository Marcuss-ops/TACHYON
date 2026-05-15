# ---------------------------------------------------------
# CLI DOMAIN: TachyonCliTests
# cli/*, command parsing, dispatch
# ---------------------------------------------------------
add_executable(TachyonCliTests
    unit/mains/test_main_cli.cpp
    unit/mains/test_stubs.cpp
    unit/cli/cli_parser_tests.cpp
)

target_compile_definitions(TachyonCliTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonCliTests
    PRIVATE
        TachyonPlatform
        TachyonTestUtils
        TachyonCore
        TachyonCLICore
        TachyonCLITools
        TachyonOperations
)

add_test(
    NAME TachyonCliTests
    COMMAND TachyonCliTests
)
tachyon_set_test_labels(TachyonCliTests cli)
