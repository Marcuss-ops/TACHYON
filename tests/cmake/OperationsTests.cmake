# ---------------------------------------------------------
# OPERATIONS DOMAIN: TachyonOperationsTests
# probe, concat, clip processing
# ---------------------------------------------------------
add_executable(TachyonOperationsTests
    unit/mains/test_main_render_pipeline.cpp
    unit/mains/test_stubs.cpp
)

target_compile_definitions(TachyonOperationsTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonOperationsTests
    PRIVATE
        TachyonPlatform
        TachyonTestUtils
        TachyonCore
        TachyonOperations
)

add_test(
    NAME TachyonOperationsTests
    COMMAND TachyonOperationsTests
)
tachyon_set_test_labels(TachyonOperationsTests operations)
