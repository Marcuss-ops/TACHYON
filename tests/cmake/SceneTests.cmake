# ---------------------------------------------------------
# SCENE DOMAIN: TachyonSceneTests
# SceneSpec, serialization, schema validation
# ---------------------------------------------------------
add_executable(TachyonSceneTests
    unit/mains/test_main_scene.cpp
    unit/mains/test_stubs.cpp
)

target_compile_definitions(TachyonSceneTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonSceneTests
    PRIVATE
        TachyonPlatform
        TachyonTestUtils
        TachyonCore
)

add_test(
    NAME TachyonSceneTests
    COMMAND TachyonSceneTests
)
tachyon_set_test_labels(TachyonSceneTests scene)
