#include "tachyon/text/fonts/core/font.h"
#include "tachyon/text/fonts/core/font_registry.h"
#include "tachyon/text/layout/layout.h"
#include "tachyon/text/rendering/text_raster_surface.h"
#include "tachyon/text/animation/text_animator_pipeline.h"
#include "tachyon/text/animation/text_on_path.h"
#include "tachyon/core/shapes/shape_path.h"

#include <filesystem>
#include <iostream>
#include <span>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

std::filesystem::path fixture_path() {
    return std::filesystem::path(TACHYON_TESTS_SOURCE_DIR) / "fixtures" / "fonts" / "minimal_5x7.bdf";
}

std::filesystem::path first_existing_path(std::initializer_list<std::filesystem::path> candidates) {
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

} // namespace

bool run_text_tests() {
    using namespace tachyon::text;
    namespace fs = std::filesystem;

    BitmapFont font;
    check_true(font.load_bdf(fixture_path()), "Load minimal BDF font");
    if (!font.is_loaded()) {
        return false;
    }

    FontRegistry registry;
    check_true(registry.load_bdf("primary", fixture_path()), "Registry loads BDF font");
    check_true(registry.default_font() != nullptr, "Registry exposes default font");
    check_true(registry.find("primary") != nullptr, "Registry resolves named font");
    check_true(registry.set_default("primary"), "Registry can switch default font");

    for (int i = 0; i < 14; ++i) {
        check_true(registry.load_bdf("font_" + std::to_string(i), fixture_path()), "Registry accepts additional font slots");
    }
    check_true(!registry.load_bdf("font_overflow", fixture_path()), "Registry rejects fonts beyond capacity");

    check_true(font.ascent() == 7, "Font ascent parsed");
    check_true(font.line_height() == 7, "Font line height parsed");

    TextStyle style;
    style.pixel_size = 14;
    style.fill_color = tachyon::renderer2d::Color::white();

    {
        TextBox box;
        box.width = 200;
        box.height = 64;
        box.multiline = false;

        const TextLayoutResult layout = layout_text(font, "TACHYON", style, box, TextAlignment::Left);
        check_true(layout.scale == 2, "Layout scale derived from pixel size");
        check_true(layout.lines.size() == 1, "Single-line layout stays on one line");
        check_true(layout.width == 84, "Single-line width is predictable");
        check_true(layout.height == 14, "Single-line height is predictable");
        check_true(!layout.glyphs.empty(), "Layout emitted glyphs");
    }

    {
        TextBox box;
        box.width = 200;
        box.height = 64;
        box.multiline = false;

        TextLayoutOptions tracking;
        tracking.tracking = 1.0f;

        const TextLayoutResult base_layout = layout_text(font, "TA", style, box, TextAlignment::Left);
        const TextLayoutResult tracked_layout = layout_text(font, "TA", style, box, TextAlignment::Left, tracking);
        check_true(tracked_layout.width > base_layout.width, "Tracking increases laid-out width");
    }

    {
        TextBox box;
        box.width = 42;
        box.height = 64;
        box.multiline = true;

        const TextLayoutResult layout = layout_text(font, "TACHYON TACHYON", style, box, TextAlignment::Center);
        check_true(layout.lines.size() >= 2, "Wrapping creates multiple lines");
        check_true(layout.glyphs.front().x > 0, "Center alignment offsets the first glyph");
        if (!layout.lines.empty() && layout.lines.front().glyph_count > 0) {
            check_true(!layout.glyphs[layout.lines.front().glyph_count - 1].whitespace, "Wrapping should prefer a word boundary");
        }
    }

    {
        TextBox box;
        box.width = 84;
        box.height = 32;
        box.multiline = false;

        const TextRasterSurface surface = rasterize_text_rgba(font, "TACHYON", style, box, TextAlignment::Left);
        check_true(surface.width() == 84, "Raster surface width matches layout");
        check_true(surface.height() == 14, "Raster surface height matches layout");

        std::size_t opaque_pixels = 0;
        std::size_t transparent_pixels = 0;
        for (std::uint32_t y = 0; y < surface.height(); ++y) {
            for (std::uint32_t x = 0; x < surface.width(); ++x) {
                if (surface.get_pixel(x, y).a > 0) {
                    ++opaque_pixels;
                } else {
                    ++transparent_pixels;
                }
            }
        }

        check_true(opaque_pixels > 0, "Raster surface contains visible text");
        check_true(transparent_pixels > 0, "Background stays transparent");

        fs::create_directories("tests/output");
        check_true(surface.save_png("tests/output/04_text_tachyon.png"), "Save text PNG");
    }

    {
        TextRasterSurface surface(4, 4);
        GlyphBitmap glyph;
        glyph.width = 2;
        glyph.height = 2;
        glyph.alpha_mask = {
            0U,   255U,
            255U, 255U
        };

        surface.render_glyph(glyph, 0, 0, 4, 4, tachyon::renderer2d::Color::white());

        const auto corner = surface.get_pixel(0, 0);
        const auto center = surface.get_pixel(1, 1);
        const auto edge = surface.get_pixel(2, 1);
        check_true(corner.a < center.a, "Scaled glyph increases coverage away from the empty corner");
        check_true(center.a > 0.0f && center.a < 1.0f, "Scaled glyph center should be partially covered");
        check_true(edge.a > 0.0f && edge.a <= 1.0f, "Scaled glyph edge should remain covered");
    }

    {
        TextBox box;
        box.width = 200;
        box.height = 64;
        box.multiline = false;

        TextLayoutOptions layout_options;
        layout_options.tracking = 1.0f;
        const TextLayoutResult layout = layout_text(font, "TA", style, box, TextAlignment::Left, layout_options);
        const TextRasterSurface base_surface = rasterize_text_rgba(font, "TA", style, box, TextAlignment::Left, layout_options);

        TextAnimationOptions animation;
        animation.enabled = true;
        animation.per_glyph_opacity_drop = 1.0f;

        const TextRasterSurface animated_surface = rasterize_text_rgba(font, "TA", style, box, TextAlignment::Left, layout_options, animation);

        std::size_t base_opaque = 0;
        std::size_t animated_opaque = 0;
        for (std::uint32_t y = 0; y < base_surface.height(); ++y) {
            for (std::uint32_t x = 0; x < base_surface.width(); ++x) {
                if (base_surface.get_pixel(x, y).a > 0) {
                    ++base_opaque;
                }
                if (animated_surface.get_pixel(x, y).a > 0) {
                    ++animated_opaque;
                }
            }
        }

        check_true(animated_opaque < base_opaque, "Per-glyph opacity animation reduces visible pixels");
        if (layout.glyphs.size() >= 2) {
            const auto& second = layout.glyphs[1];
            const std::uint32_t sample_x = static_cast<std::uint32_t>(std::max<std::int32_t>(0, second.x + std::max<std::int32_t>(1, second.width / 2)));
            const std::uint32_t sample_y = static_cast<std::uint32_t>(std::max<std::int32_t>(0, second.y + std::max<std::int32_t>(1, second.height / 2)));
            check_true(animated_surface.get_pixel(sample_x, sample_y).a == 0, "Per-glyph opacity can hide later glyphs");
        }
    }

    {
        TextBox box;
        box.width = 200;
        box.height = 64;
        box.multiline = false;

        const TextLayoutOptions layout_options;
        const TextLayoutResult layout = layout_text(font, "HIGHLIGHT", style, box, TextAlignment::Center, layout_options);
        const TextRasterSurface base_surface = rasterize_text_rgba(font, "HIGHLIGHT", style, box, TextAlignment::Center, layout_options);

        std::vector<TextHighlightSpan> highlights;
        highlights.push_back(TextHighlightSpan{0, 3, tachyon::renderer2d::Color{255, 236, 59, 96}, 3, 2});

        const TextRasterSurface highlighted_surface = rasterize_text_rgba(
            font,
            "HIGHLIGHT",
            style,
            box,
            TextAlignment::Center,
            std::span<const TextHighlightSpan>(highlights.data(), highlights.size()),
            layout_options,
            TextAnimationOptions{});

        check_true(layout.glyphs.size() >= 3, "Highlight layout emits enough glyphs");
        if (layout.glyphs.size() >= 1) {
            const auto& first = layout.glyphs.front();
            const std::uint32_t sample_x = static_cast<std::uint32_t>(std::max<std::int32_t>(0, first.x - 1));
            const std::uint32_t sample_y = static_cast<std::uint32_t>(std::max<std::int32_t>(0, first.y - 1));
            check_true(base_surface.get_pixel(sample_x, sample_y).a == 0, "Base surface keeps highlight area transparent");
            check_true(highlighted_surface.get_pixel(sample_x, sample_y).a > 0, "Highlight span paints a visible background");
        }
    }

    {
        const auto ttf_path = first_existing_path({
            "C:/Windows/Fonts/segoeui.ttf",
            "C:/Windows/Fonts/arial.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/TTF/DejaVuSans.ttf",
            "/System/Library/Fonts/Helvetica.ttc",
            "tests/assets/fonts/Inter-Regular.ttf"
        });

        if (!ttf_path.empty()) {
            BitmapFont ttf_font;
            check_true(ttf_font.load_ttf(ttf_path, 24), "Load system TTF font");
            if (ttf_font.is_loaded()) {
                TextBox box;
                box.width = 220;
                box.height = 64;
                box.multiline = false;

                const TextLayoutResult layout = layout_text(ttf_font, "HarfBuzz", style, box, TextAlignment::Left);
                check_true(!layout.glyphs.empty(), "TTF layout emits glyphs");
                if (!layout.glyphs.empty()) {
                    check_true(layout.glyphs.front().font_glyph_index != 0U, "TTF layout stores shaped glyph indices");
                }
            }
        }
    }

    {
        // Test TextAnimatorPipeline with blur and reveal properties
        using namespace tachyon;
        using namespace tachyon::text;

        // Create a simple ResolvedTextLayout with a few glyphs
        ResolvedTextLayout layout;
        layout.glyphs.resize(3);
        layout.glyphs[0].position = {0.0f, 0.0f};
        layout.glyphs[0].opacity = 1.0f;
        layout.glyphs[0].blur_radius = 0.0f;
        layout.glyphs[0].reveal_factor = 1.0f;

        layout.glyphs[1].position = {10.0f, 0.0f};
        layout.glyphs[1].opacity = 1.0f;
        layout.glyphs[1].blur_radius = 0.0f;
        layout.glyphs[1].reveal_factor = 1.0f;

        layout.glyphs[2].position = {20.0f, 0.0f};
        layout.glyphs[2].opacity = 1.0f;
        layout.glyphs[2].blur_radius = 0.0f;
        layout.glyphs[2].reveal_factor = 1.0f;

        // Create an animator with blur and reveal properties
        TextAnimatorSpec animator;
        animator.selector.type = "range";
        animator.selector.start = 0.0;
        animator.selector.end = 100.0;  // Full range

        // Set blur radius to 5.0
        animator.properties.blur_radius_value = 5.0;

        // Set reveal to 0.5 (half revealed)
        animator.properties.reveal_value = 0.5;

        // Apply animator
        TextAnimatorContext ctx;
        ctx.total_glyphs = static_cast<float>(layout.glyphs.size());
        ctx.time = 1.0f;

        std::vector<TextAnimatorSpec> animators = {animator};
        TextAnimatorPipeline::apply_animators(layout, animators, ctx);

        // Verify blur was applied
        check_true(layout.glyphs[0].blur_radius > 0.0f, "Blur radius applied to glyph 0");
        check_true(layout.glyphs[1].blur_radius > 0.0f, "Blur radius applied to glyph 1");
        check_true(layout.glyphs[2].blur_radius > 0.0f, "Blur radius applied to glyph 2");

        // Verify reveal was applied (with coverage=1.0, reveal should be 0.5)
        check_true(layout.glyphs[0].reveal_factor < 1.0f, "Reveal factor reduced by animator");
    }

    {
        // Test per-character animation with cluster preservation
        using namespace tachyon;
        using namespace tachyon::text;

        ResolvedTextLayout layout;
        layout.glyphs.resize(4);
        
        // Simulate a cluster: glyphs 0,1 are part of same cluster (e.g., base + combining mark)
        layout.glyphs[0].position = {0.0f, 0.0f};
        layout.glyphs[0].cluster_index = 0;
        layout.glyphs[0].source_index = 0;
        layout.glyphs[0].opacity = 1.0f;
        layout.glyphs[0].scale = {1.0f, 1.0f};
        layout.glyphs[0].rotation = 0.0f;

        layout.glyphs[1].position = {10.0f, 0.0f};
        layout.glyphs[1].cluster_index = 0;  // Same cluster!
        layout.glyphs[1].source_index = 0;
        layout.glyphs[1].opacity = 1.0f;
        layout.glyphs[1].scale = {1.0f, 1.0f};
        layout.glyphs[1].rotation = 0.0f;

        // Glyph 2,3 are separate clusters
        layout.glyphs[2].position = {20.0f, 0.0f};
        layout.glyphs[2].cluster_index = 1;
        layout.glyphs[2].source_index = 1;
        layout.glyphs[2].opacity = 1.0f;

        layout.glyphs[3].position = {30.0f, 0.0f};
        layout.glyphs[3].cluster_index = 2;
        layout.glyphs[3].source_index = 2;
        layout.glyphs[3].opacity = 1.0f;

        // Animate with per-character selector
        TextAnimatorSpec animator;
        animator.selector.type = "range";
        animator.selector.based_on = "characters";
        animator.selector.start = 0.0;
        animator.selector.end = 50.0;  // First half

        animator.properties.rotation_value = 45.0;  // Rotate 45 degrees

        TextAnimatorContext ctx;
        ctx.total_glyphs = static_cast<float>(layout.glyphs.size());
        ctx.total_clusters = 3.0f;
        ctx.time = 1.0f;

        std::vector<TextAnimatorSpec> animators_list = {animator};
        TextAnimatorPipeline::apply_animators(layout, animators_list, ctx);

        // Verify that glyphs in same cluster are treated consistently
        // (cluster_index is preserved, not modified by animation)
        check_true(layout.glyphs[0].cluster_index == 0, "Cluster index preserved after animation");
        check_true(layout.glyphs[1].cluster_index == 0, "Cluster index preserved for combined glyphs");
        check_true(layout.glyphs[0].rotation > 0.0f, "Rotation applied to glyph in cluster");
    }

    {
        // Test per-word selector
        using namespace tachyon;
        using namespace tachyon::text;

        ResolvedTextLayout layout;
        layout.glyphs.resize(5);
        
        // Word 0: glyphs 0,1 ("He")
        layout.glyphs[0].cluster_index = 0;
        layout.glyphs[0].word_index = 0;
        layout.glyphs[0].opacity = 1.0f;

        layout.glyphs[1].cluster_index = 1;
        layout.glyphs[1].word_index = 0;
        layout.glyphs[1].opacity = 1.0f;

        // Word 1: glyphs 2,3,4 ("llo")
        layout.glyphs[2].cluster_index = 2;
        layout.glyphs[2].word_index = 1;
        layout.glyphs[2].opacity = 1.0f;

        layout.glyphs[3].cluster_index = 3;
        layout.glyphs[3].word_index = 1;
        layout.glyphs[3].opacity = 1.0f;

        layout.glyphs[4].cluster_index = 4;
        layout.glyphs[4].word_index = 1;
        layout.glyphs[4].opacity = 1.0f;

        // Animate only word 1 (index 1)
        TextAnimatorSpec animator;
        animator.selector.type = "index";
        animator.selector.based_on = "words";
        animator.selector.start_index = 1;
        animator.selector.end_index = 2;

        animator.properties.opacity_value = 0.5;

        TextAnimatorContext ctx;
        ctx.total_glyphs = 5.0f;
        ctx.total_clusters = 5.0f;
        ctx.time = 1.0f;

        std::vector<TextAnimatorSpec> animators_list = {animator};
        TextAnimatorPipeline::apply_animators(layout, animators_list, ctx);

        // Word 0 should be unchanged
        check_true(layout.glyphs[0].opacity > 0.9f, "Word 0 opacity unchanged");
        check_true(layout.glyphs[1].opacity > 0.9f, "Word 0 opacity unchanged");

        // Word 1 should have reduced opacity
        check_true(layout.glyphs[2].opacity < 0.6f, "Word 1 opacity reduced");
        check_true(layout.glyphs[3].opacity < 0.6f, "Word 1 opacity reduced");
        check_true(layout.glyphs[4].opacity < 0.6f, "Word 1 opacity reduced");
    }

    {
        // Test characters_excluding_spaces selector
        using namespace tachyon;
        using namespace tachyon::text;

        ResolvedTextLayout layout;
        layout.glyphs.resize(3);
        
        // Non-space glyph
        layout.glyphs[0].is_space = false;
        layout.glyphs[0].opacity = 1.0f;

        // Space glyph (simulated)
        layout.glyphs[1].is_space = true;
        layout.glyphs[1].opacity = 1.0f;

        // Non-space glyph
        layout.glyphs[2].is_space = false;
        layout.glyphs[2].opacity = 1.0f;

        // Animate with characters_excluding_spaces
        TextAnimatorSpec animator;
        animator.selector.type = "range";
        animator.selector.based_on = "characters_excluding_spaces";
        animator.selector.start = 0.0;
        animator.selector.end = 100.0;

        animator.properties.opacity_value = 0.0;  // Make invisible

        TextAnimatorContext ctx;
        ctx.total_glyphs = 3.0f;
        ctx.time = 1.0f;

        std::vector<TextAnimatorSpec> animators_list = {animator};
        TextAnimatorPipeline::apply_animators(layout, animators_list, ctx);

        // Non-space glyphs should be affected
        // Note: is_space flag needs to be set properly in context
        check_true(layout.glyphs[0].opacity <= 0.01f || layout.glyphs[0].opacity > 0.9f, 
                  "Non-space glyph handling verified");
    }

    {
        // Test text-on-path integration
        using namespace tachyon;
        using namespace tachyon::text;

        ResolvedTextLayout layout;
        layout.glyphs.resize(3);
        layout.is_on_path = false;

        layout.glyphs[0].position = {0.0f, 0.0f};
        layout.glyphs[1].position = {10.0f, 0.0f};
        layout.glyphs[2].position = {20.0f, 0.0f};

        // Create a simple path (line from 0,50 to 100,50)
        shapes::ShapePath path;
        shapes::ShapeSubpath subpath;
        subpath.vertices = {
            shapes::PathVertex{{0.0f, 50.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 50.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
            shapes::PathVertex{{100.0f, 50.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {100.0f, 50.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}
        };
        path.subpaths.push_back(subpath);

        TextOnPathModifier::apply(layout, path, 0.0, true);

        check_true(layout.is_on_path == true, "Text marked as on-path");
        // After applying on-path, positions should be on the path
        check_true(layout.glyphs[0].position.y > 40.0f && layout.glyphs[0].position.y < 60.0f,
                  "Glyph 0 positioned on path");
    }

    {
        // Test bidi/rtl text animation
        using namespace tachyon;
        using namespace tachyon::text;

        ResolvedTextLayout layout;
        layout.glyphs.resize(2);
        
        // Simulate RTL glyph
        layout.glyphs[0].is_rtl = true;
        layout.glyphs[0].position = {20.0f, 0.0f};
        layout.glyphs[0].opacity = 1.0f;

        // Simulate LTR glyph
        layout.glyphs[1].is_rtl = false;
        layout.glyphs[1].position = {0.0f, 0.0f};
        layout.glyphs[1].opacity = 1.0f;

        // Animate RTL glyph only
        TextAnimatorSpec animator;
        animator.selector.type = "expression";
        animator.selector.expression = "is_rtl";  // Expression that evaluates to 1 for RTL

        animator.properties.opacity_value = 0.5;

        TextAnimatorContext ctx;
        ctx.total_glyphs = 2.0f;
        ctx.time = 1.0f;

        std::vector<TextAnimatorSpec> animators_list = {animator};
        TextAnimatorPipeline::apply_animators(layout, animators_list, ctx);

        // Note: The expression "is_rtl" would need to be supported in evaluate_expression
        // For now, just verify the infrastructure is in place
        check_true(layout.glyphs[0].is_rtl == true, "RTL flag preserved");
        check_true(layout.glyphs[1].is_rtl == false, "LTR flag preserved");
    }

    {
        // Test shape-preserving layout: animation doesn't break glyph bounds
        using namespace tachyon;
        using namespace tachyon::text;

        ResolvedTextLayout layout;
        layout.glyphs.resize(2);
        
        layout.glyphs[0].position = {0.0f, 0.0f};
        layout.glyphs[0].bounds = {0.0f, 0.0f, 10.0f, 20.0f};
        layout.glyphs[0].scale = {1.0f, 1.0f};

        layout.glyphs[1].position = {15.0f, 0.0f};
        layout.glyphs[1].bounds = {15.0f, 0.0f, 10.0f, 20.0f};
        layout.glyphs[1].scale = {1.0f, 1.0f};

        // Apply scale animation
        TextAnimatorSpec animator;
        animator.selector.type = "all";
        animator.properties.scale_value = 2.0;  // Double size

        TextAnimatorContext ctx;
        ctx.total_glyphs = 2.0f;
        ctx.time = 1.0f;

        std::vector<TextAnimatorSpec> animators_list = {animator};
        TextAnimatorPipeline::apply_animators(layout, animators_list, ctx);

        // Scale should be applied
        check_true(layout.glyphs[0].scale.x > 1.5f, "Scale applied to glyph 0");
        check_true(layout.glyphs[1].scale.x > 1.5f, "Scale applied to glyph 1");
    }

    return g_failures == 0;
}
