# Header smoke tests - catch header errors early without full builds
add_executable(HeaderSmokeCompositionSpec compile/header_smoke_composition_spec.cpp)
target_link_libraries(HeaderSmokeCompositionSpec PRIVATE TachyonCore TachyonScene)

add_executable(HeaderSmokeSceneSpec compile/header_smoke_scene_spec.cpp)
target_link_libraries(HeaderSmokeSceneSpec PRIVATE TachyonCore TachyonScene)

add_executable(HeaderSmokeLayerSpec compile/header_smoke_layer_spec.cpp)
target_link_libraries(HeaderSmokeLayerSpec PRIVATE TachyonCore TachyonScene)

add_executable(HeaderSmokeSceneBuilder compile/header_smoke_scene_builder.cpp)
target_link_libraries(HeaderSmokeSceneBuilder PRIVATE TachyonCore TachyonScene)

add_executable(HeaderSmokeRenderJobSpec compile/header_smoke_render_job_spec.cpp)
target_link_libraries(HeaderSmokeRenderJobSpec PRIVATE TachyonCore TachyonScene TachyonRuntime)

add_custom_target(HeaderSmokeTests
    DEPENDS HeaderSmokeCompositionSpec HeaderSmokeSceneSpec HeaderSmokeLayerSpec HeaderSmokeSceneBuilder HeaderSmokeRenderJobSpec
    COMMENT "Running header smoke tests to catch compilation errors early"
)

add_executable(Phase1Tests unit/text/phase1_tests.cpp)
target_link_libraries(Phase1Tests PRIVATE TachyonCore TachyonScene TachyonText)
target_compile_definitions(Phase1Tests PRIVATE TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}")

add_test(
    NAME TachyonSmokeTests
    COMMAND TachyonTests
)
tachyon_set_test_labels(TachyonSmokeTests smoke)

set_tests_properties(TachyonSmokeTests PROPERTIES
    ENVIRONMENT "TACHYON_TEST_FILTER=math,property,expression,camera_cuts"
)
