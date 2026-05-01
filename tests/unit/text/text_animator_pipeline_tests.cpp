#include <gtest/gtest.h>
#include "tachyon/text/animation/text_animator_pipeline.h"
#include "tachyon/text/animation/text_animator_utils.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/text/core/layout/resolved_text_layout.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace tachyon::text {

// Helper to create a simple ResolvedGlyph
ResolvedGlyph make_glyph(std::uint32_t codepoint, std::size_t glyph_index, std::size_t cluster_index = 0) {
    ResolvedGlyph g;
    g.codepoint = codepoint;
    g.font_glyph_index = codepoint;
    g.cluster_index = cluster_index;
    g.source_index = glyph_index;
    g.word_index = 0;
    g.line_index = 0;
    g.is_space = (codepoint == 32);
    g.font_size = 32.0f;
    g.fill_color = {255, 255, 255, 255};
    g.opacity = 1.0f;
    g.scale = {1.0f, 1.0f};
    g.rotation = 0.0f;
    g.blur_radius = 0.0f;
    g.reveal_factor = 1.0f;
    g.position = {static_cast<float>(glyph_index * 20), 0.0f};
    return g;
}

TEST(TextAnimatorPipelineTest, EmptyLayout) {
    ResolvedTextLayout layout;
    std::vector<TextAnimatorSpec> animators(1);
    TextAnimatorContext ctx;
    ctx.time = 0.5f;

    TextAnimatorPipeline::apply_animators(layout, animators, ctx);
    EXPECT_TRUE(layout.glyphs.empty());
}

TEST(TextAnimatorPipelineTest, RangeSelectorCoverage) {
    ResolvedTextLayout layout;
    layout.glyphs.push_back(make_glyph('A', 0));
    layout.glyphs.push_back(make_glyph('B', 1));
    layout.glyphs.push_back(make_glyph('C', 2));

    TextAnimatorSpec animator;
    animator.selector.type = "range";
    animator.selector.start = 0.0;
    animator.selector.end = 100.0;

    std::vector<TextAnimatorSpec> animators = {animator};
    TextAnimatorContext ctx;
    ctx.time = 0.5f;
    ctx.total_glyphs = 3.0f;

    TextAnimatorPipeline::apply_animators(layout, animators, ctx);
}

TEST(TextAnimatorPipelineTest, IndexSelector) {
    ResolvedTextLayout layout;
    layout.glyphs.push_back(make_glyph('A', 0));
    layout.glyphs.push_back(make_glyph('B', 1));
    layout.glyphs.push_back(make_glyph('C', 2));

    TextAnimatorSpec animator;
    animator.selector.type = "index";
    animator.selector.start_index = 1;
    animator.selector.end_index = 2;
    animator.properties.opacity_value = 0.5;

    std::vector<TextAnimatorSpec> animators = {animator};
    TextAnimatorContext ctx;
    ctx.time = 1.0f;
    ctx.total_glyphs = 3.0f;

    TextAnimatorPipeline::apply_animators(layout, animators, ctx);

    EXPECT_FLOAT_EQ(layout.glyphs[0].opacity, 1.0f);
    EXPECT_FLOAT_EQ(layout.glyphs[1].opacity, 0.5f);
}

TEST(TextAnimatorPipelineTest, PositionOffsetAnimation) {
    ResolvedTextLayout layout;
    layout.glyphs.push_back(make_glyph('A', 0));

    TextAnimatorSpec animator;
    animator.selector.type = "all";
    animator.properties.position_offset_value = math::Vector2{10.0, 20.0};

    std::vector<TextAnimatorSpec> animators = {animator};
    TextAnimatorContext ctx;
    ctx.time = 1.0f;
    ctx.total_glyphs = 1.0f;

    float original_x = layout.glyphs[0].position.x;
    TextAnimatorPipeline::apply_animators(layout, animators, ctx);

    EXPECT_FLOAT_EQ(layout.glyphs[0].position.x, original_x + 10.0f);
    EXPECT_FLOAT_EQ(layout.glyphs[0].position.y, 20.0f);
}

TEST(TextAnimatorPipelineTest, ScaleAnimation) {
    ResolvedTextLayout layout;
    layout.glyphs.push_back(make_glyph('A', 0));

    TextAnimatorSpec animator;
    animator.selector.type = "all";
    animator.properties.scale_value = 2.0;

    std::vector<TextAnimatorSpec> animators = {animator};
    TextAnimatorContext ctx;
    ctx.time = 1.0f;
    ctx.total_glyphs = 1.0f;

    TextAnimatorPipeline::apply_animators(layout, animators, ctx);

    EXPECT_FLOAT_EQ(layout.glyphs[0].scale.x, 2.0f);
    EXPECT_FLOAT_EQ(layout.glyphs[0].scale.y, 2.0f);
}

TEST(TextAnimatorPipelineTest, RotationAnimation) {
    ResolvedTextLayout layout;
    layout.glyphs.push_back(make_glyph('A', 0));

    TextAnimatorSpec animator;
    animator.selector.type = "all";
    animator.properties.rotation_value = 45.0;

    std::vector<TextAnimatorSpec> animators = {animator};
    TextAnimatorContext ctx;
    ctx.time = 1.0f;
    ctx.total_glyphs = 1.0f;

    TextAnimatorPipeline::apply_animators(layout, animators, ctx);
    EXPECT_FLOAT_EQ(layout.glyphs[0].rotation, 45.0f);
}

TEST(TextAnimatorPipelineTest, OpacityAnimation) {
    ResolvedTextLayout layout;
    layout.glyphs.push_back(make_glyph('A', 0));
    layout.glyphs[0].opacity = 1.0f;

    TextAnimatorSpec animator;
    animator.selector.type = "all";
    animator.properties.opacity_value = 0.5;

    std::vector<TextAnimatorSpec> animators = {animator};
    TextAnimatorContext ctx;
    ctx.time = 1.0f;
    ctx.total_glyphs = 1.0f;

    TextAnimatorPipeline::apply_animators(layout, animators, ctx);
    EXPECT_FLOAT_EQ(layout.glyphs[0].opacity, 0.5f);
}

TEST(TextAnimatorPipelineTest, BlurRadiusAnimation) {
    ResolvedTextLayout layout;
    layout.glyphs.push_back(make_glyph('A', 0));

    TextAnimatorSpec animator;
    animator.selector.type = "all";
    animator.properties.blur_radius_value = 5.0;

    std::vector<TextAnimatorSpec> animators = {animator};
    TextAnimatorContext ctx;
    ctx.time = 1.0f;
    ctx.total_glyphs = 1.0f;

    TextAnimatorPipeline::apply_animators(layout, animators, ctx);
    EXPECT_FLOAT_EQ(layout.glyphs[0].blur_radius, 5.0f);
}

TEST(TextAnimatorPipelineTest, RevealAnimation) {
    ResolvedTextLayout layout;
    layout.glyphs.push_back(make_glyph('A', 0));
    layout.glyphs[0].reveal_factor = 1.0f;

    TextAnimatorSpec animator;
    animator.selector.type = "all";
    animator.properties.reveal_value = 0.0;

    std::vector<TextAnimatorSpec> animators = {animator};
    TextAnimatorContext ctx;
    ctx.time = 1.0f;
    ctx.total_glyphs = 1.0f;

    TextAnimatorPipeline::apply_animators(layout, animators, ctx);
    EXPECT_FLOAT_EQ(layout.glyphs[0].reveal_factor, 0.0f);
}

TEST(TextAnimatorPipelineTest, TrackingAnimation) {
    ResolvedTextLayout layout;
    layout.glyphs.push_back(make_glyph('A', 0));
    layout.glyphs.push_back(make_glyph('B', 1));

    TextAnimatorSpec animator;
    animator.selector.type = "all";
    animator.properties.tracking_amount_value = 10.0;

    std::vector<TextAnimatorSpec> animators = {animator};
    TextAnimatorContext ctx;
    ctx.time = 1.0f;
    ctx.total_glyphs = 2.0f;

    float original_x_0 = layout.glyphs[0].position.x;
    float original_x_1 = layout.glyphs[1].position.x;

    TextAnimatorPipeline::apply_animators(layout, animators, ctx);

    EXPECT_GE(layout.glyphs[0].position.x, original_x_0);
    EXPECT_GT(layout.glyphs[1].position.x, original_x_1);
}

TEST(TextAnimatorPipelineTest, StaggerModeCharacter) {
    ResolvedTextLayout layout;
    for (int i = 0; i < 5; ++i) {
        layout.glyphs.push_back(make_glyph('A' + i, i));
    }

    TextAnimatorSpec animator;
    animator.selector.type = "all";
    animator.selector.stagger_mode = "character";
    animator.selector.stagger_delay = 0.1;
    animator.properties.opacity_value = 0.0;

    std::vector<TextAnimatorSpec> animators = {animator};

    TextAnimatorContext ctx;
    ctx.time = 0.15f;
    ctx.total_glyphs = 5.0f;

    TextAnimatorPipeline::apply_animators(layout, animators, ctx);

    // At t=0.15, glyph 0 has staggered_t = 0.15 (opacity 0.0 * 1.0 coverage = 0.0)
    // Actually with static value 0.0 and full coverage, result is 0.0
    EXPECT_FLOAT_EQ(layout.glyphs[0].opacity, 0.0f);
}

TEST(TextAnimatorPipelineTest, SpaceHandling) {
    ResolvedTextLayout layout;
    layout.glyphs.push_back(make_glyph('A', 0));
    layout.glyphs.push_back(make_glyph(32, 1));
    layout.glyphs[1].is_space = true;
    layout.glyphs.push_back(make_glyph('B', 2));

    TextAnimatorSpec animator;
    animator.selector.type = "all";
    animator.selector.based_on = "characters_excluding_spaces";
    animator.properties.opacity_value = 0.0;

    std::vector<TextAnimatorSpec> animators = {animator};

    TextAnimatorContext ctx;
    ctx.time = 1.0f;
    ctx.total_glyphs = 3.0f;
    ctx.total_non_space_glyphs = 2.0f;
    ctx.non_space_glyph_index = 0.0f;

    layout.glyphs[0].opacity = 1.0f;
    layout.glyphs[1].opacity = 1.0f;
    layout.glyphs[2].opacity = 1.0f;

    TextAnimatorPipeline::apply_animators(layout, animators, ctx);

    // Space glyph should not be affected by characters_excluding_spaces
    EXPECT_FLOAT_EQ(layout.glyphs[1].opacity, 1.0f);
    // Non-space glyphs should have opacity animated to 0.0
    EXPECT_FLOAT_EQ(layout.glyphs[0].opacity, 0.0f);
}

TEST(TextAnimatorPipelineTest, FillColorAnimation) {
    ResolvedTextLayout layout;
    layout.glyphs.push_back(make_glyph('A', 0));

    TextAnimatorSpec animator;
    animator.selector.type = "all";
    animator.properties.fill_color_value = ColorSpec{255, 0, 0, 255};

    std::vector<TextAnimatorSpec> animators = {animator};
    TextAnimatorContext ctx;
    ctx.time = 1.0f;
    ctx.total_glyphs = 1.0f;

    TextAnimatorPipeline::apply_animators(layout, animators, ctx);
    EXPECT_EQ(layout.glyphs[0].fill_color.r, 255);
    EXPECT_EQ(layout.glyphs[0].fill_color.g, 0);
}

TEST(TextAnimatorPipelineTest, MultipleAnimators) {
    ResolvedTextLayout layout;
    layout.glyphs.push_back(make_glyph('A', 0));

    TextAnimatorSpec anim1;
    anim1.selector.type = "all";
    anim1.properties.opacity_value = 0.5;

    TextAnimatorSpec anim2;
    anim2.selector.type = "all";
    anim2.properties.scale_value = 2.0;

    std::vector<TextAnimatorSpec> animators = {anim1, anim2};
    TextAnimatorContext ctx;
    ctx.time = 1.0f;
    ctx.total_glyphs = 1.0f;

    TextAnimatorPipeline::apply_animators(layout, animators, ctx);

    EXPECT_FLOAT_EQ(layout.glyphs[0].opacity, 0.5f);
    EXPECT_FLOAT_EQ(layout.glyphs[0].scale.x, 2.0f);
}

TEST(TextAnimatorPipelineTest, CoverageBasedBlending) {
    ResolvedTextLayout layout;
    layout.glyphs.push_back(make_glyph('A', 0));
    layout.glyphs[0].opacity = 1.0f;

    TextAnimatorSpec animator;
    animator.selector.type = "range";
    animator.selector.start = 0.0;
    animator.selector.end = 50.0;
    animator.properties.opacity_value = 0.0;

    std::vector<TextAnimatorSpec> animators = {animator};
    TextAnimatorContext ctx;
    ctx.time = 1.0f;
    ctx.total_glyphs = 1.0f;

    TextAnimatorPipeline::apply_animators(layout, animators, ctx);
    // With square shape and range [0%, 50%], glyph at position 0 is within range so coverage = 1.0
    // opacity = 1.0 * (1 - 1.0) + 0.0 * 1.0 = 0.0
    EXPECT_FLOAT_EQ(layout.glyphs[0].opacity, 0.0f);
}

TEST(TextAnimatorPipelineTest, LigatureClusterHandling) {
    ResolvedTextLayout layout;
    
    auto g1 = make_glyph('f', 0, 0);
    auto g2 = make_glyph('i', 1, 0);
    
    layout.glyphs.push_back(g1);
    layout.glyphs.push_back(g2);
    
    GlyphCluster cluster;
    cluster.source_text_start = 0;
    cluster.source_text_length = 2;
    cluster.glyph_start = 0;
    cluster.glyph_count = 2;
    layout.clusters.push_back(cluster);

    TextAnimatorSpec animator;
    animator.selector.type = "all";
    animator.properties.opacity_value = 0.0;

    std::vector<TextAnimatorSpec> animators = {animator};
    TextAnimatorContext ctx;
    ctx.time = 1.0f;
    ctx.total_glyphs = 2.0f;

    layout.glyphs[0].opacity = 1.0f;
    layout.glyphs[1].opacity = 1.0f;

    TextAnimatorPipeline::apply_animators(layout, animators, ctx);

    EXPECT_FLOAT_EQ(layout.glyphs[0].opacity, layout.glyphs[1].opacity);
    EXPECT_FLOAT_EQ(layout.glyphs[0].opacity, 0.0f);
}

TEST(TextAnimatorPipelineTest, MultiGlyphClusterPositionOffset) {
    ResolvedTextLayout layout;
    
    for (int i = 0; i < 3; ++i) {
        auto g = make_glyph('A' + i, i, 0);
        layout.glyphs.push_back(g);
    }
    
    GlyphCluster cluster;
    cluster.source_text_start = 0;
    cluster.source_text_length = 3;
    cluster.glyph_start = 0;
    cluster.glyph_count = 3;
    layout.clusters.push_back(cluster);

    TextAnimatorSpec animator;
    animator.selector.type = "all";
    animator.properties.position_offset_value = math::Vector2{100.0f, 50.0f};

    std::vector<TextAnimatorSpec> animators = {animator};
    TextAnimatorContext ctx;
    ctx.time = 1.0f;
    ctx.total_glyphs = 3.0f;

    TextAnimatorPipeline::apply_animators(layout, animators, ctx);

    // Position offset is added to the glyph's position
    for (int i = 0; i < 3; ++i) {
        EXPECT_FLOAT_EQ(layout.glyphs[i].position.x, i * 20.0f + 100.0f);
        EXPECT_FLOAT_EQ(layout.glyphs[i].position.y, 50.0f);
    }
}

} // namespace tachyon::text
