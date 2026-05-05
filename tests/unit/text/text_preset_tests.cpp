#include "tachyon/text/animation/text_presets.h"
#include "tachyon/presets/text/text_builders.h"
#include "tachyon/presets/text/text_params.h"

#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

} // namespace

bool run_text_preset_tests() {
    using namespace tachyon;
    using namespace tachyon::text;
    using namespace tachyon::presets;

    {
        const auto animator = make_bounce_in_animator();
        check_true(animator.name == "BounceIn", "BounceIn animator name");
        check_true(animator.selector.based_on == "characters_excluding_spaces", "BounceIn based_on");
        check_true(animator.selector.stagger_mode == "character", "BounceIn stagger mode");
        check_true(!animator.properties.opacity_keyframes.empty(), "BounceIn opacity keyframes");
        check_true(!animator.properties.position_offset_keyframes.empty(), "BounceIn position keyframes");
        check_true(!animator.properties.scale_keyframes.empty(), "BounceIn scale keyframes");
    }

    {
        const auto animator = make_word_punch_animator();
        check_true(animator.name == "WordPunch", "WordPunch animator name");
        check_true(animator.selector.based_on == "words", "WordPunch based_on");
        check_true(animator.selector.stagger_mode == "word", "WordPunch stagger mode");
        check_true(!animator.properties.opacity_keyframes.empty(), "WordPunch opacity keyframes");
        check_true(!animator.properties.scale_keyframes.empty(), "WordPunch scale keyframes");
        check_true(!animator.properties.blur_radius_keyframes.empty(), "WordPunch blur keyframes");
    }

    {
        TextParams params;
        params.text = "Hello world";
        params.animation = "tachyon.textanim.bounce_in";
        params.stagger_delay = 0.025;
        params.reveal_duration = 0.45;

        const auto layer = build_text(params);
        check_true(layer.text_animators.size() == 1, "build_text bounce_in animator count");
        check_true(layer.text_animators[0].name == "BounceIn", "build_text bounce_in animator name");
    }

    {
        TextParams params;
        params.text = "Hello world";
        params.animation = "tachyon.textanim.word_punch";
        params.stagger_delay = 0.06;
        params.reveal_duration = 0.35;

        const auto layer = build_text(params);
        check_true(layer.text_animators.size() == 1, "build_text word_punch animator count");
        check_true(layer.text_animators[0].name == "WordPunch", "build_text word_punch animator name");
    }

    return g_failures == 0;
}
