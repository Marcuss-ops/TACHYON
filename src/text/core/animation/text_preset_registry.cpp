#include "tachyon/text/animation/text_preset_registry.h"
#include "tachyon/text/animation/text_presets.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <utility>
#include <algorithm>

namespace tachyon::text {

struct TextPresetRegistry::Impl {
    std::unordered_map<std::string, TextPresetSpec> presets;
    std::vector<std::string> index;
};

TextPresetRegistry::TextPresetRegistry() : m_impl(std::make_unique<Impl>()) {
    // Register built-in presets with lambda adapters for signature differences
    register_preset({"fade_in", "Fade In", "Simple fade in animation",
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return make_fade_in_animator(based_on, stagger_delay, reveal_duration);
                     }});

    register_preset({"slide_in", "Slide In", "Slide in from offset",
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return make_slide_in_animator(based_on, stagger_delay, 28.0, reveal_duration);
                     }});

    register_preset({"pop_in", "Pop In", "Pop in with scale and slide",
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return make_pop_in_animator(based_on, stagger_delay, 18.0, reveal_duration);
                     }});

    register_preset({"typewriter", "Typewriter", "Character-by-character reveal",
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         // Typewriter uses characters_per_second = 1.0/stagger_delay
                         double cps = stagger_delay > 0 ? 1.0 / stagger_delay : 20.0;
                         return make_typewriter_animator(cps, "|");
                     }});

    register_preset({"kinetic_blur", "Kinetic Blur", "Motion blur slide in",
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return make_kinetic_blur_animator(200.0, reveal_duration);
                     }});

    register_preset({"blur_to_focus", "Blur to Focus", "Blur radius fade out",
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return make_blur_to_focus_animator(based_on, reveal_duration, 8.0);
                     }});

    register_preset({"minimal_fade_up", "Minimal Fade Up", "Fade up with Y offset",
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return make_minimal_fade_up_animator(based_on, reveal_duration, 12.0);
                     }});

    register_preset({"tracking_reveal", "Tracking Reveal", "Letter spacing reveal",
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return make_tracking_reveal_animator(based_on, reveal_duration, 40.0);
                     }});

    register_preset({"soft_scale_in", "Soft Scale In", "Scale up from small",
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return make_soft_scale_in_animator(based_on, reveal_duration, 0.95);
                     }});

    register_preset({"subtle_y_rotate", "Subtle Y Rotate", "Rotation on Y axis",
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return make_subtle_y_rotate_animator(based_on, reveal_duration, 5.0);
                     }});

    register_preset({"fill_wipe", "Fill Wipe", "Color fill wipe",
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return make_fill_wipe_animator(based_on, reveal_duration);
                     }});

    register_preset({"outline_to_solid", "Outline to Solid", "Outline width to zero",
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return make_outline_to_solid_animator(based_on, reveal_duration, 1.0);
                     }});

    register_preset({"phrase_intro", "Phrase Intro", "Slide in for phrases",
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return make_phrase_intro_animator(based_on, stagger_delay, 28.0, reveal_duration);
                     }});

    register_preset({"numeric_intro", "Numeric Intro", "Pop in for numbers",
                     [](const std::string& based_on, double stagger_delay, double reveal_duration) {
                         return make_numeric_intro_animator(based_on, stagger_delay, 18.0, reveal_duration);
                     }});
}

TextPresetRegistry::~TextPresetRegistry() = default;

TextPresetRegistry& TextPresetRegistry::instance() {
    static TextPresetRegistry registry;
    return registry;
}

void TextPresetRegistry::register_preset(const TextPresetSpec& spec) {
    if (m_impl->presets.find(spec.id) == m_impl->presets.end()) {
        m_impl->index.push_back(spec.id);
    }
    m_impl->presets[spec.id] = spec;
}

void TextPresetRegistry::unregister_preset(const std::string& id) {
    m_impl->presets.erase(id);
    auto it = std::find(m_impl->index.begin(), m_impl->index.end(), id);
    if (it != m_impl->index.end()) {
        m_impl->index.erase(it);
    }
}

const TextPresetSpec* TextPresetRegistry::find(const std::string& id) const {
    auto it = m_impl->presets.find(id);
    if (it != m_impl->presets.end()) {
        return &it->second;
    }
    return nullptr;
}

std::size_t TextPresetRegistry::count() const {
    return m_impl->presets.size();
}

const TextPresetSpec* TextPresetRegistry::get_by_index(std::size_t index) const {
    if (index < m_impl->index.size()) {
        return &m_impl->presets.at(m_impl->index[index]);
    }
    return nullptr;
}

TextAnimatorSpec TextPresetRegistry::create(const std::string& id,
                                            const std::string& based_on,
                                            double stagger_delay,
                                            double reveal_duration) const {
    const TextPresetSpec* spec = find(id);
    if (spec && spec->factory) {
        return spec->factory(based_on, stagger_delay, reveal_duration);
    }
    // Fallback to fade_in
    return make_fade_in_animator(based_on, stagger_delay, reveal_duration);
}

} // namespace tachyon::text
