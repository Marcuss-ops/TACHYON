#pragma once

#include "tachyon/runtime/core/diagnostics.h"

#include <filesystem>
#include <string>
#include <vector>

namespace tachyon::text {

/// A single subtitle cue parsed from an SRT file.
struct SubtitleEntry {
    int index{0};             ///< 1-based sequential index
    double start_seconds{0.0}; ///< Cue start time in seconds
    double end_seconds{0.0};   ///< Cue end time in seconds
    std::string text;          ///< Multi-line cue text (lines joined with '\n')
};

/// Parse an SRT subtitle file.
/// Handles both Unix (\n) and Windows (\r\n) line endings.
/// Timing format: "HH:MM:SS,mmm --> HH:MM:SS,mmm"
[[nodiscard]] ::tachyon::ParseResult<std::vector<SubtitleEntry>>
parse_srt(const std::filesystem::path& path);

/// Return the subtitle entry active at the given time, or nullptr.
/// Returns the first entry whose [start, end) interval contains @p time_seconds.
[[nodiscard]] const SubtitleEntry*
find_active_subtitle(const std::vector<SubtitleEntry>& entries, double time_seconds);

} // namespace tachyon::text
