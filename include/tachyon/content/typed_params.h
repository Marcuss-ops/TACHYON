#pragma once

#include <string>

namespace tachyon::content {

/**
 * @brief Parameters for the Typewriter text animator.
 */
enum class TypewriterUnit {
    Character,
    Word,
    Line,
    Sentence
};

enum class TypewriterStyle {
    Classic,
    Soft,
    Archive,
    Terminal
};

struct TypewriterParams {
    double speed = 20.0; // CPS, WPS, etc. depending on unit
    std::string cursor = "|";
    TypewriterUnit unit = TypewriterUnit::Character;
    TypewriterStyle style = TypewriterStyle::Classic;
    bool show_cursor = true;
};

} // namespace tachyon::content
