# Tachyon Presets Module

set(TachyonPresetsSources
    animation2d/animation2d_preset_registry.cpp
    animation2d/animation2d_presets_table.cpp
    background/background_presets.cpp
    background/background_builders.cpp
    background/background_resolver.cpp
    builders_common.cpp
    image/image_builders.cpp
    shape/shape_builders.cpp
)

if(TACHYON_ENABLE_TEXT)
    list(APPEND TachyonPresetsSources
        text/text_manifest.cpp
        text/text_builders.cpp
        text/text_registry.cpp
    )
endif()

list(APPEND TachyonPresetsSources
    transition/transition_preset_registry.cpp
    transition/transition_builders.cpp
    transition/transition_presets_table.cpp
    effects/effect_preset_registry.cpp
    effects/blur_effects.cpp
    effects/color_effects.cpp
    effects/distortion_effects.cpp
    effects/generator_effects.cpp
    effects/stylize_effects.cpp
    effects/transition_effects.cpp
    scene/scene_preset_registry.cpp
    preset_scene_resolver.cpp
    common_scenes.cpp
    preset_applier.cpp
)

tachyon_add_module(
    NAME Presets
    SOURCES ${TachyonPresetsSources}
    PUBLIC_DEPS TachyonCore TachyonCatalog
)

if(TACHYON_ENABLE_TEXT)
    target_link_libraries(TachyonPresets PUBLIC TachyonText)
endif()

tachyon_link_text_deps(TachyonPresets)
tachyon_link_media_deps(TachyonPresets)
tachyon_link_omp(TachyonPresets)
