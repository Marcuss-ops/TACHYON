#pragma once

#include <cstdint>
#include <vector>
#include <cstddef>

namespace tachyon::text {

class GlyphAtlas {
public:
    GlyphAtlas() = default;
    ~GlyphAtlas() = default;

    GlyphAtlas(const GlyphAtlas&) = delete;
    GlyphAtlas& operator=(const GlyphAtlas&) = delete;
    GlyphAtlas(GlyphAtlas&& other) noexcept = default;
    GlyphAtlas& operator=(GlyphAtlas&& other) noexcept = default;

    struct Allocation {
        std::uint32_t x{0};
        std::uint32_t y{0};
        std::uint32_t width{0};
        std::uint32_t height{0};
    };

    Allocation allocate(std::uint32_t width, std::uint32_t height,
                        const std::uint8_t* data, std::size_t data_size);

    void clear();

    const std::vector<std::uint8_t>& data() const { return m_data; }
    std::uint32_t width() const { return m_width; }
    std::uint32_t height() const { return m_height; }

private:
    struct Shelf {
        std::uint32_t y{0};
        std::uint32_t height{0};
        std::uint32_t current_x{0};
    };

    std::vector<std::uint8_t> m_data;
    std::uint32_t m_width{1024};
    std::uint32_t m_height{1024};
    std::vector<Shelf> m_shelves;

    Shelf* find_shelf(std::uint32_t width, std::uint32_t height);
    void expand_atlas(std::uint32_t new_width, std::uint32_t new_height);
};

} // namespace tachyon::text
