#pragma once

#include "tachyon/text/font.h"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

namespace tachyon::text {

class FontRegistry {
public:
    static constexpr std::size_t kMaxFonts = 15;

    bool load_bdf(const std::string& name, const std::filesystem::path& path);
    bool load_ttf(const std::string& name, const std::filesystem::path& path, std::uint32_t pixel_size);

    const Font* find(const std::string& name) const;
    Font* find(const std::string& name);

    const Font* default_font() const;
    bool set_default(const std::string& name);

    std::size_t size() const;

private:
    bool can_add_new_font(const std::string& name) const;

    std::unordered_map<std::string, Font> m_fonts;
    std::string m_default_name;
};

} // namespace tachyon::text
