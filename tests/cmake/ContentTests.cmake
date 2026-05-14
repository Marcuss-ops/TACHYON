# ---------------------------------------------------------
# 5. CONTENT DOMAIN: TachyonContentTests
# content/presets/*, library, text presets
# ---------------------------------------------------------
add_executable(TachyonContentTests
    unit/mains/test_main_content.cpp
    unit/core/library/library_tests.cpp
    unit/core/library/transition_manifest_audit_tests.cpp
    unit/core/library/transition_descriptor_alignment_tests.cpp
    unit/core/library/missing_transition_fallback_tests.cpp
    unit/core/library/preview_dev_workflow_tests.cpp
    unit/core/spec/layerspec_schema_tests.cpp
    unit/core/spec/scene_validator_normalization_tests.cpp
    unit/text/glyph_cache_tests.cpp
    unit/text/text_tests.cpp
    unit/text/text_preset_tests.cpp
    unit/text/text_registry_tests.cpp
    unit/text/text_animation_sampler_tests.cpp
    unit/presets/text_domain_boundary_tests.cpp
    unit/audio/test_audio_pitch_correct.cpp
    unit/audio/test_audio_trim.cpp
    unit/presets/background_kind_registry_tests.cpp
    unit/presets/transition_builder_tests.cpp
    unit/presets/sfx_contract_tests.cpp
    unit/presets/effect_preset_registry_tests.cpp
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
        TachyonPresets
        TachyonLibrary
        TachyonRenderer2D
        TachyonPlatform
        TachyonRuntime
)

if(TACHYON_ENABLE_TEXT)
    target_link_libraries(TachyonContentTests PRIVATE TachyonText)
endif()

if(TACHYON_ENABLE_AUDIO)
    target_link_libraries(TachyonContentTests PRIVATE TachyonAudio)
endif()

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

# PR 1: Strict Preset Tests
add_executable(transition_preset_strict_tests unit/presets/transition/transition_preset_strict_tests.cpp)
target_link_libraries(transition_preset_strict_tests PRIVATE 
    TachyonCore 
    TachyonScene 
    TachyonPresets 
    TachyonTestUtils
    TachyonRenderer2D
    TachyonPlatform
)
add_test(NAME transition_preset_strict COMMAND transition_preset_strict_tests)
tachyon_set_test_labels(transition_preset_strict content)
