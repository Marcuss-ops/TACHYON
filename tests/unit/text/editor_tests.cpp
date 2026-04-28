#include "tachyon/text/animation/text_editor.h"
#include "tachyon/text/animation/text_presets.h"
#include "tachyon/text/animation/text_scene_presets.h"
#include <gtest/gtest.h>

namespace tachyon::text {

class TextEditorTest : public ::testing::Test {
protected:
    TextEditor editor;
};

TEST_F(TextEditorTest, BasicInsertion) {
    editor.set_text("Hello");
    EXPECT_EQ(editor.get_text(), "Hello");
    EXPECT_EQ(editor.get_document().cluster_count(), 5);
}

TEST_F(TextEditorTest, GraphemeSafety_CombiningMarks) {
    // A + Combining Acute Accent
    std::string text = "A\xCC\x81"; 
    editor.set_text(text);
    EXPECT_EQ(editor.get_document().cluster_count(), 1);
    
    // Backspace should delete the whole cluster (A + accent)
    editor.delete_backward();
    EXPECT_EQ(editor.get_text(), "");
}

TEST_F(TextEditorTest, GraphemeSafety_Emoji) {
    // Family: Man, Woman, Girl, Boy with ZWJ
    // ðŸ‘¨â€ðŸ‘©â€ðŸ‘§â€ðŸ‘¦
    std::string family = "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7\xE2\x80\x8D\xF0\x9F\x91\xA6";
    editor.set_text(family);
    
    // The iterator should keep the ZWJ family sequence in one cluster.
    EXPECT_EQ(editor.get_document().cluster_count(), 1);
    
    editor.delete_backward();
    EXPECT_EQ(editor.get_text(), "");
}

TEST_F(TextEditorTest, SelectionAndDeletion) {
    editor.set_text("ABCDE");
    editor.set_selection(1, 4); // Select "BCD"
    editor.delete_selection();
    EXPECT_EQ(editor.get_text(), "AE");
}

TEST_F(TextEditorTest, CaretMovement) {
    editor.set_text("AB");
    editor.set_selection(0, 0);
    
    editor.move_caret_right(false);
    EXPECT_EQ(editor.get_selection().focus, 1);
    EXPECT_EQ(editor.get_selection().anchor, 1);
    
    editor.move_caret_right(true); // Shift pressed
    EXPECT_EQ(editor.get_selection().focus, 2);
    EXPECT_EQ(editor.get_selection().anchor, 1);
    
    editor.move_caret_left(false);
    EXPECT_EQ(editor.get_selection().focus, 1);
    EXPECT_EQ(editor.get_selection().anchor, 1);
}

TEST_F(TextEditorTest, WordBasedMovement) {
    editor.set_text("Hello world testing");
    editor.set_selection(0, 0);
    
    editor.move_caret_word_right(false);
    EXPECT_EQ(editor.get_selection().focus, 5); // End of "Hello"
    
    editor.move_caret_word_right(false);
    EXPECT_EQ(editor.get_selection().focus, 11); // End of "world"
    
    editor.move_caret_word_left(false);
    EXPECT_EQ(editor.get_selection().focus, 6); // Start of "world"
}

TEST_F(TextEditorTest, VisualIME) {
    editor.set_text("AB");
    editor.set_selection(1, 1);
    
    editor.set_composition("C", 1);
    EXPECT_EQ(editor.get_display_text(), "ACB");
    EXPECT_EQ(editor.get_text(), "AB");
    
    editor.commit_composition();
    EXPECT_EQ(editor.get_text(), "ACB");
}

TEST_F(TextEditorTest, WordBasedMovement_Unicode) {
    // "Hello Ø¹Ø§Ù„Ù… (world) Ø§Ø®ØªØ¨Ø§Ø± (test)"
    std::string text = "Hello \xD8\xB9\xD8\xA7\xD9\x84\xD9\x85 \xD8\xA7\xD8\xAE\xD8\xAA\xD8\xA8\xD8\xA7\xD8\xB1";
    editor.set_text(text);
    editor.set_selection(0, 0);
    
    editor.move_caret_word_right(false);
    EXPECT_EQ(editor.get_selection().focus, 5); // End of "Hello"
    
    editor.move_caret_word_right(false);
    EXPECT_EQ(editor.get_selection().focus, 10); // End of Arabic "world"
}

TEST_F(TextEditorTest, BiDi_Basics) {
    // Mixed LTR and RTL
    std::string mixed = "ABC \xD8\xB9\xD8\xA7\xD9\x84\xD9\x85"; // ABC [Arabic world]
    editor.set_text(mixed);
    
    // Cluster count: 3 Latin (ABC) + 1 space + 4 Arabic = 8 clusters
    EXPECT_EQ(editor.get_document().cluster_count(), 8);
    
    // Deleting backward at the end of RTL run (removes last Arabic cluster)
    editor.set_selection(8, 8);
    editor.delete_backward();
    EXPECT_EQ(editor.get_document().cluster_count(), 7);
}

TEST(TextPresetTest, MinimalBackgroundBoxIsEnabledAndSubtle) {
    const auto box = make_minimal_text_background_box();
    EXPECT_TRUE(box.enabled);
    EXPECT_LT(box.fill_color.a, 1.0f);
    EXPECT_EQ(box.padding_left, box.padding_right);
    EXPECT_EQ(box.padding_top, box.padding_bottom);
}

TEST(TextPresetTest, MinimalFadeUpAnimatesOpacityAndYOffset) {
    const auto anim = make_minimal_fade_up_animator();
    EXPECT_EQ(anim.name, "MinimalFadeUp");
    EXPECT_FALSE(anim.properties.opacity_keyframes.empty());
    EXPECT_FALSE(anim.properties.position_offset_keyframes.empty());
}

TEST(TextPresetTest, SplitLineStaggerUsesLineBasedStaggering) {
    const auto anim = make_split_line_stagger_animator();
    EXPECT_EQ(anim.selector.based_on, "lines");
    EXPECT_EQ(anim.selector.stagger_mode, "line");
    EXPECT_GT(anim.selector.stagger_delay, 0.0);
}

TEST(TextPresetTest, EnhanceTextSceneBuildsBackgroundAndTextLayers) {
    const auto scene = make_enhance_text_scene();
    ASSERT_EQ(scene.compositions.size(), 1u);

    const auto& comp = scene.compositions.front();
    EXPECT_TRUE(comp.background.has_value());
    ASSERT_EQ(comp.layers.size(), 2u);
    EXPECT_TRUE(comp.layers[0].procedural.has_value());
    EXPECT_EQ(comp.layers[0].procedural->kind, "aura");
    EXPECT_EQ(comp.layers[1].type, "text");
    EXPECT_EQ(comp.layers[1].text_content, "Il testo leggero entra, respira e resta pulito.");
    EXPECT_EQ(comp.layers[1].text_animators.size(), 3u);
}

} // namespace tachyon::text
