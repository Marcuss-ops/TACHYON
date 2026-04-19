#include "tachyon/text/font_registry.h"

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

} // namespace tachyon::text
