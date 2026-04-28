#include "tachyon/text/fonts/core/font_registry.h"

#include <algorithm>
#include <utility>

namespace tachyon::text {

bool FontRegistry::can_add_new_font(const std::string& name) const {
    if (m_fonts.find(name) != m_fonts.end()) {
        return true;
    }
    return m_fonts.size() < kMaxFonts;
}

bool FontRegistry::load_bdf(const std::string& name, const std::filesystem::path& path) {
    if (!can_add_new_font(name)) {
        return false;
    }

    Font font;
    if (!font.load_bdf(path)) {
        return false;
    }

    m_fonts.erase(name);
    m_fonts.emplace(name, std::move(font));
    if (m_default_name.empty()) {
        m_default_name = name;
    }
    return true;
}

bool FontRegistry::load_ttf(const std::string& name, const std::filesystem::path& path, std::uint32_t pixel_size) {
    if (!can_add_new_font(name)) {
        return false;
    }

    Font font;
    if (!font.load_ttf(path, pixel_size)) {
        return false;
    }

    m_fonts.erase(name);
    m_fonts.emplace(name, std::move(font));
    if (m_default_name.empty()) {
        m_default_name = name;
    }
    return true;
}

const Font* FontRegistry::find(const std::string& name) const {
    const auto it = m_fonts.find(name);
    return it != m_fonts.end() ? &it->second : nullptr;
}

Font* FontRegistry::find(const std::string& name) {
    const auto it = m_fonts.find(name);
    return it != m_fonts.end() ? &it->second : nullptr;
}

const Font* FontRegistry::find_by_id(std::uint64_t id) const {
    for (const auto& [name, font] : m_fonts) {
        if (font.id() == id) {
            return &font;
        }
    }
    return nullptr;
}

const Font* FontRegistry::default_font() const {
    if (m_default_name.empty()) {
        return nullptr;
    }
    return find(m_default_name);
}

bool FontRegistry::set_default(const std::string& name) {
    if (m_fonts.find(name) == m_fonts.end()) {
        return false;
    }

    m_default_name = name;
    return true;
}

std::size_t FontRegistry::size() const {
    return m_fonts.size();
}

void FontRegistry::add_fallback(const std::string& font_name, const std::string& fallback_name) {
    m_fallbacks[font_name].push_back(fallback_name);
}

std::vector<const Font*> FontRegistry::get_fallback_chain(const std::string& font_name) const {
    std::vector<const Font*> chain;
    
    const Font* primary = find(font_name);
    if (primary) {
        chain.push_back(primary);
    }

    const auto it = m_fallbacks.find(font_name);
    if (it != m_fallbacks.end()) {
        for (const auto& fallback_name : it->second) {
            const Font* fallback = find(fallback_name);
            if (fallback) {
                chain.push_back(fallback);
            }
        }
    }

    // Always add default font as ultimate fallback if not already in chain
    const Font* def = default_font();
    if (def && std::find(chain.begin(), chain.end(), def) == chain.end()) {
        chain.push_back(def);
    }

    return chain;
}

} // namespace tachyon::text

