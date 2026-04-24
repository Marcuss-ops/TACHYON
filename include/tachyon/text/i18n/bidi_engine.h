#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include <cstdint>

namespace tachyon::text {

enum class CharacterDirection {
    // Strong types
    LTR,      // Left-to-Right
    RTL,      // Right-to-Left
    AL,       // Arabic Letter (strong RTL)
    // Weak types
    EN,       // European Number
    AN,       // Arabic Number
    ES,       // European Separator
    CS,       // Common Separator
    NSM,      // Non-Spacing Mark
    // Neutral types
    Neutral,  // Default neutral
    B,        // Paragraph Separator
    S,        // Segment Separator
    WS,       // White Space
    ON,       // Other Neutral
    // Explicit directional controls
    LRE,      // Left-to-Right Embedding
    RLE,      // Right-to-Left Embedding
    LRO,      // Left-to-Right Override
    RLO,      // Right-to-Left Override
    PDF       // Pop Directional Format
};

struct BidiRun {
    std::size_t start;
    std::size_t length;
    CharacterDirection direction;
    int level{0};
};

/**
 * @brief Unicode Bidirectional Engine with heuristic coverage.
 * 
 * Note: This is a heuristic implementation of UAX#9, NOT a full
 * compliant BiDi algorithm. It lacks full bracket pair resolution,
 * explicit directional overrides (LRO/RLO), and complex neutral
 * resolution rules. It is sufficient for basic LTR/RTL text mixing
 * but may fail on complex typographic layouts.
 */
class BidiEngine {
public:
    /**
     * @brief Analyzes a UTF-32 string and returns Bidi runs.
     * 
     * This is a heuristic implementation of the Unicode Bidirectional Algorithm.
     */
    static std::vector<BidiRun> analyze(const std::vector<std::uint32_t>& codepoints);

private:
    static CharacterDirection get_direction(std::uint32_t codepoint);
    static std::vector<BidiRun> analyze_paragraph(const std::vector<std::uint32_t>& codepoints, std::size_t start, std::size_t length);
};

} // namespace tachyon::text
