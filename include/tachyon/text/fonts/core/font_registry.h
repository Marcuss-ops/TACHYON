#pragma once

#include "tachyon/text/fonts/core/font.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace tachyon::text {

class FontRegistry {
public:
    static constexpr std::size_t kMaxFonts = std::numeric_limits<std::size_t>::max();

    bool load_bdf(const std::string& name, const std::filesystem::path& path);
    bool load_ttf(const std::string& name, const std::filesystem::path& path, std::uint32_t pixel_size);

    const Font* find(const std::string& name) const;
    Font* find(const std::string& name);
    const Font* find_by_id(std::uint64_t id) const;
    const Font* default_font() const;
    bool set_default(const std::string& name);

    void add_fallback(const std::string& font_name, const std::string& fallback_name);
    std::vector<const Font*> get_fallback_chain(const std::string& font_name) const;

    std::size_t size() const;

private:
    bool can_add_new_font(const std::string& name) const;

    std::unordered_map<std::string, Font> m_fonts;
    std::unordered_map<std::string, std::vector<std::string>> m_fallbacks;
    std::string m_default_name;
};

} // namespace tachyon::text

