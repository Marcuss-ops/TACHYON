# Tachyon Text Module

set(TachyonTextSources
    text_measure.cpp
    core/TextLayerSpec.cpp
    bidi/bidi_engine.cpp
    bidi/script_detector.cpp
    core/animation/text_animator_context.cpp
    core/animation/text_animator_coverage.cpp
    core/animation/text_animator_expression.cpp
    core/animation/text_animator_paints.cpp
    core/animation/text_animator_sampling.cpp
    core/animation/text_animator_utils.cpp
    core/animation/text_anim_backend.cpp
    core/animation/text_presets.cpp
    core/animation/text_scene_presets.cpp
    core/content/subtitle.cpp
    core/content/word_timestamps.cpp
    core/layout/layout.cpp
    core/layout/layout_cache.cpp
    core/layout/layout_engine.cpp
    core/layout/layout_engine_helpers.cpp
    core/rendering/outline_extractor.cpp
    core/rendering/text_raster_surface.cpp
    core/fonts/core/embedded_fallback_font.cpp
    core/fonts/core/font.cpp
    core/fonts/core/font_face.cpp
    core/fonts/core/font_instance.cpp
    core/fonts/core/font_registry.cpp
    core/fonts/management/font_downloader.cpp
    core/fonts/management/font_manifest.cpp
    core/fonts/management/font_preloader.cpp
    core/fonts/management/font_resolver.cpp
    core/fonts/utils/font_coverage_reporter.cpp
    core/fonts/utils/glyph_atlas.cpp
    ../experimental/text_editor/text_editor.cpp
    core/low_level/freetype/freetype_manager.cpp
    core/low_level/glyph/glyph_loader.cpp
    core/low_level/utils/font_utils.cpp
    core/low_level/shaping/font_shaping.cpp
    core/low_level/shaping/shaping_cache.cpp
    core/low_level/utf8/utf8_decoder.cpp
    animation/text_animation_sampler.cpp
)

tachyon_add_module(
    NAME Text
    SOURCES ${TachyonTextSources}
    PUBLIC_DEPS TachyonCore
)

# Text dependencies
tachyon_link_text_deps(TachyonText)
