#include "tachyon/renderer2d/text/utf8/utf8_decoder.h"

namespace tachyon::renderer2d::text::utf8 {

std::vector<std::uint32_t> decode_utf8(std::string_view text) {
    std::vector<std::uint32_t> codepoints;
    codepoints.reserve(text.size());

    for (std::size_t i = 0; i < text.size();) {
        const unsigned char lead = static_cast<unsigned char>(text[i]);
        if ((lead & 0x80U) == 0U) {
            codepoints.push_back(lead);
            ++i;
            continue;
        }

        if ((lead & 0xE0U) == 0xC0U && i + 1 < text.size()) {
            const auto cp =
                ((static_cast<std::uint32_t>(lead & 0x1FU)) << 6U) |
                (static_cast<std::uint32_t>(static_cast<unsigned char>(text[i + 1]) & 0x3FU));
            codepoints.push_back(cp);
            i += 2;
            continue;
        }

        if ((lead & 0xF0U) == 0xE0U && i + 2 < text.size()) {
            const auto cp =
                ((static_cast<std::uint32_t>(lead & 0x0FU)) << 12U) |
                ((static_cast<std::uint32_t>(static_cast<unsigned char>(text[i + 1]) & 0x3FU)) << 6U) |
                (static_cast<std::uint32_t>(static_cast<unsigned char>(text[i + 2]) & 0x3FU));
            codepoints.push_back(cp);
            i += 3;
            continue;
        }

        if ((lead & 0xF8U) == 0xF0U && i + 3 < text.size()) {
            const auto cp =
                ((static_cast<std::uint32_t>(lead & 0x07U)) << 18U) |
                ((static_cast<std::uint32_t>(static_cast<unsigned char>(text[i + 1]) & 0x3FU)) << 12U) |
                ((static_cast<std::uint32_t>(static_cast<unsigned char>(text[i + 2]) & 0x3FU)) << 6U) |
                (static_cast<std::uint32_t>(static_cast<unsigned char>(text[i + 3]) & 0x3FU));
            codepoints.push_back(cp);
            i += 4;
            continue;
        }

        codepoints.push_back(static_cast<std::uint32_t>('?'));
        ++i;
    }

    return codepoints;
}

} // namespace tachyon::renderer2d::text::utf8