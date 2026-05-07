# ---------------------------------------------------------
# 5. CONTENT DOMAIN: TachyonContentTests
# content/presets/*, catalog, text presets
# ---------------------------------------------------------
add_executable(TachyonContentTests
    unit/test_main_content.cpp
    unit/core/catalog/catalog_tests.cpp
    unit/core/catalog/transition_catalog_audit_tests.cpp
    unit/core/catalog/transition_descriptor_alignment_tests.cpp
    unit/core/catalog/background_catalog_alignment_tests.cpp
    unit/core/catalog/missing_transition_fallback_tests.cpp
    unit/core/catalog/preview_dev_workflow_tests.cpp
    unit/core/spec/layerspec_schema_tests.cpp
    unit/core/spec/scene_validator_normalization_tests.cpp
    unit/text/glyph_cache_tests.cpp
    unit/text/text_tests.cpp
    unit/text/text_preset_tests.cpp
    unit/text/text_animator_preset_registry_tests.cpp
    unit/presets/text_domain_boundary_tests.cpp
    unit/audio/test_audio_pitch_correct.cpp
    unit/audio/test_audio_trim.cpp
    unit/presets/background_kind_registry_tests.cpp
    unit/presets/background_preset_registry_tests.cpp
    unit/presets/background_resolver_tests.cpp
    unit/presets/transition_builder_tests.cpp
    unit/presets/sfx_contract_tests.cpp
    unit/presets/transition_preset_registry_tests.cpp
    unit/presets/transition_runtime_tests.cpp
    unit/presets/preset_audit_tests.cpp
)

target_compile_definitions(TachyonContentTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonContentTests
    PRIVATE
        TachyonTestUtils
        TachyonCore
        TachyonScene
        TachyonText
        TachyonAudio
        TachyonCatalog
        TachyonRenderer2D
        TachyonPlatform
        TachyonRuntime
)

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(TachyonContentTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

if(MSVC)
    target_compile_options(TachyonContentTests PRIVATE /FS)
endif()

add_test(
    NAME TachyonContentTests
    COMMAND TachyonContentTests
)
tachyon_set_test_labels(TachyonContentTests core)
