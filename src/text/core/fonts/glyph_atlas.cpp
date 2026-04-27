#include "tachyon/text/fonts/glyph_atlas.h"
#include <algorithm>
#include <cstring>

namespace tachyon::text {

GlyphAtlas::Allocation GlyphAtlas::allocate(std::uint32_t width, std::uint32_t height,
                                            const std::uint8_t* data, std::size_t data_size) {
    Shelf* shelf = find_shelf(width, height);
    if (!shelf) {
        expand_atlas(m_width, m_height * 2);
        shelf = find_shelf(width, height);
    }

    if (!shelf) {
        return {};
    }

    Allocation alloc;
    alloc.x = shelf->current_x;
    alloc.y = shelf->y;
    alloc.width = width;
    alloc.height = height;

    std::size_t dst_offset = (alloc.y * m_width) + alloc.x;
    for (std::uint32_t y = 0; y < height; ++y) {
        std::memcpy(m_data.data() + dst_offset + y * m_width,
                    data + y * width,
                    width);
    }

    shelf->current_x += width;
    return alloc;
}

void GlyphAtlas::clear() {
    m_data.clear();
    m_shelves.clear();
    m_width = 1024;
    m_height = 1024;
}

GlyphAtlas::Shelf* GlyphAtlas::find_shelf(std::uint32_t width, std::uint32_t height) {
    for (auto& shelf : m_shelves) {
        if (shelf.height >= height && (m_width - shelf.current_x) >= width) {
            return &shelf;
        }
    }

    if (m_height - (m_shelves.empty() ? 0 : m_shelves.back().y + m_shelves.back().height) >= height) {
        m_shelves.push_back({m_shelves.empty() ? 0 : m_shelves.back().y + m_shelves.back().height, height, 0});
        m_shelves.back().current_x = width;
        return &m_shelves.back();
    }

    return nullptr;
}

void GlyphAtlas::expand_atlas(std::uint32_t new_width, std::uint32_t new_height) {
    std::vector<std::uint8_t> new_data(new_width * new_height, 0);
    for (std::uint32_t y = 0; y < m_height; ++y) {
        std::memcpy(new_data.data() + y * new_width,
                    m_data.data() + y * m_width,
                    m_width);
    }
    m_data = std::move(new_data);
    m_width = new_width;
    m_height = new_height;
}

} // namespace tachyon::text
