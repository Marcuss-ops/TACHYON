#include <gtest/gtest.h>
#include "tachyon/text/animation/text_animator.h"

using namespace tachyon;
using namespace tachyon::text;

class TextAnimationSamplerTest : public ::testing::Test {
protected:
    TextLayoutResult create_mock_layout(const std::string& text) {
        TextLayoutResult layout;
        std::size_t word_idx = 0;
        for (std::size_t i = 0; i < text.length(); ++i) {
            PositionedGlyph g;
            g.codepoint = static_cast<std::uint32_t>(text[i]);
            g.word_index = word_idx;
            g.opacity = 1.0f;
            g.scale = {1.0f, 1.0f};
            g.position = {static_cast<float>(i) * 10.0f, 0.0f};
            g.blur_radius = 0.0f;
            
            if (text[i] == ' ') {
                g.whitespace = true;
                word_idx++;
            }
            layout.glyphs.push_back(g);
        }
        return layout;
    }

    TextAnimatorSpec create_typewriter_spec(double stagger, const std::string& based_on = "characters") {
        TextAnimatorSpec anim;
        anim.selector.based_on = based_on;
        anim.selector.stagger_delay = stagger;
        anim.selector.shape = "square";
        anim.properties.opacity_value = 0.0;
        anim.selector.offset = 0.0;
        return anim;
    }
};

// 1. Basic Typewriter (Character by Character, Square)
TEST_F(TextAnimationSamplerTest, Variation1_BasicCharacter) {
    auto layout = create_mock_layout("HELLO");
    auto anim = create_typewriter_spec(0.1);
    
    // At t=0, only 'H' should be visible? 
    // Wait, with my logic: t=0, index=0, delay=0, progress=0. Square shape says progress >= 0 -> coverage=0.
    // So 'H' is visible.
    TextAnimationSampler::sample(layout, {anim}, 0.0);
    EXPECT_NEAR(layout.glyphs[0].opacity, 1.0f, 0.01f);
    EXPECT_NEAR(layout.glyphs[1].opacity, 0.0f, 0.01f);
    
    // At t=0.25, 'H', 'E', 'L' should be visible
    TextAnimationSampler::sample(layout, {anim}, 0.25);
    EXPECT_NEAR(layout.glyphs[0].opacity, 1.0f, 0.01f);
    EXPECT_NEAR(layout.glyphs[1].opacity, 1.0f, 0.01f);
    EXPECT_NEAR(layout.glyphs[2].opacity, 1.0f, 0.01f);
    EXPECT_NEAR(layout.glyphs[3].opacity, 0.0f, 0.01f);
}

// 2. Word Reveal
TEST_F(TextAnimationSamplerTest, Variation2_WordReveal) {
    auto layout = create_mock_layout("HI YOU");
    auto anim = create_typewriter_spec(0.5, "words");
    
    TextAnimationSampler::sample(layout, {anim}, 0.1);
    EXPECT_NEAR(layout.glyphs[0].opacity, 1.0f, 0.01f); // 'H'
    EXPECT_NEAR(layout.glyphs[1].opacity, 1.0f, 0.01f); // 'I'
    EXPECT_NEAR(layout.glyphs[3].opacity, 0.0f, 0.01f); // 'Y'
    
    TextAnimationSampler::sample(layout, {anim}, 0.6);
    EXPECT_NEAR(layout.glyphs[3].opacity, 1.0f, 0.01f); // 'Y'
}

// 3. Smooth Fade (Ramp)
TEST_F(TextAnimationSamplerTest, Variation3_SmoothFade) {
    auto layout = create_mock_layout("HELLO");
    auto anim = create_typewriter_spec(0.1);
    anim.selector.shape = "ramp_up";
    
    // At t=0.15, 'H' is fully visible, 'E' is halfway (progress = 0.15 - 0.1 = 0.05. 0.05 / 0.1 = 0.5 coverage)
    // Wait, my duration is 0.1 in the code.
    TextAnimationSampler::sample(layout, {anim}, 0.15);
    EXPECT_NEAR(layout.glyphs[0].opacity, 1.0f, 0.01f);
    EXPECT_NEAR(layout.glyphs[1].opacity, 0.5f, 0.1f);
}

// 4. Reveal with Position Offset (Slide Up)
TEST_F(TextAnimationSamplerTest, Variation4_SlideUp) {
    auto layout = create_mock_layout("A");
    TextAnimatorSpec anim;
    anim.selector.stagger_delay = 0.1;
    anim.properties.position_offset_value = math::Vector2{0, 20};
    anim.properties.opacity_value = 0.0;
    
    // Start of animation
    TextAnimationSampler::sample(layout, {anim}, -0.05);
    EXPECT_NEAR(layout.glyphs[0].position.y, 20.0f, 0.01f);
    EXPECT_NEAR(layout.glyphs[0].opacity, 0.0f, 0.01f);
}

// 5. Reveal with Blur
TEST_F(TextAnimationSamplerTest, Variation5_BlurReveal) {
    auto layout = create_mock_layout("A");
    TextAnimatorSpec anim;
    anim.properties.blur_radius_value = 10.0;
    
    TextAnimationSampler::sample(layout, {anim}, -0.1);
    EXPECT_NEAR(layout.glyphs[0].blur_radius, 10.0f, 0.01f);
}

// 6. Scale Pop
TEST_F(TextAnimationSamplerTest, Variation6_ScalePop) {
    auto layout = create_mock_layout("A");
    TextAnimatorSpec anim;
    anim.properties.scale_value = 0.0;
    
    TextAnimationSampler::sample(layout, {anim}, -0.1);
    EXPECT_NEAR(layout.glyphs[0].scale.x, 0.0f, 0.01f);
}

// 7. Fast Stagger
TEST_F(TextAnimationSamplerTest, Variation7_FastStagger) {
    auto layout = create_mock_layout("ABCDE");
    auto anim = create_typewriter_spec(0.01);
    
    TextAnimationSampler::sample(layout, {anim}, 0.05);
    for(int i=0; i<5; ++i) EXPECT_NEAR(layout.glyphs[i].opacity, 1.0f, 0.01f);
}

// 8. Long Duration Fade
TEST_F(TextAnimationSamplerTest, Variation8_LongFade) {
    auto layout = create_mock_layout("A");
    auto anim = create_typewriter_spec(0.1);
    anim.selector.shape = "ramp_up";
    // Default duration is 0.1 in my current impl
    TextAnimationSampler::sample(layout, {anim}, 0.05);
    EXPECT_NEAR(layout.glyphs[0].opacity, 0.5f, 0.1f);
}

// 9. Delayed Start
TEST_F(TextAnimationSamplerTest, Variation9_DelayedStart) {
    auto layout = create_mock_layout("A");
    auto anim = create_typewriter_spec(0.1);
    anim.selector.offset = 1.0;
    
    TextAnimationSampler::sample(layout, {anim}, 0.5);
    EXPECT_NEAR(layout.glyphs[0].opacity, 0.0f, 0.01f);
}

// 10. Multiple Animators (Fade + Slide)
TEST_F(TextAnimationSamplerTest, Variation10_MultipleAnimators) {
    auto layout = create_mock_layout("A");
    auto anim1 = create_typewriter_spec(0.1);
    TextAnimatorSpec anim2;
    anim2.properties.position_offset_value = math::Vector2{10, 0};
    
    TextAnimationSampler::sample(layout, {anim1, anim2}, 0.0);
    EXPECT_NEAR(layout.glyphs[0].opacity, 1.0f, 0.01f);
    EXPECT_NEAR(layout.glyphs[0].position.x, 0.0f, 0.01f);
}
