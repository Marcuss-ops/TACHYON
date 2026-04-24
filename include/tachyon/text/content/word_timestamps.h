#pragma once

#include "tachyon/runtime/core/diagnostics/diagnostics.h"

#include <filesystem>
#include <string>
#include <vector>

namespace tachyon::text {

/// A single word timestamp entry from STT output.
struct WordTimestamp {
    std::string word;      ///< The word text
    double start{0.0};    ///< Start time in seconds
    double end{0.0};      ///< End time in seconds
};

/// A track containing word-level timestamps for audio-highlight synchronization.
struct WordTimestampTrack {
    std::vector<WordTimestamp> words;
};

/// Parse a JSON file containing word timestamps.
/// Expected format:
/// {
///   "words": [
///     {"word": "Ciao", "start": 0.24, "end": 0.61},
///     {"word": "mondo", "start": 0.65, "end": 1.02}
///   ]
/// }
/// The source of the JSON is external (Whisper, AssemblyAI, or any STT system).
[[nodiscard]] ::tachyon::ParseResult<WordTimestampTrack>
parse_word_timestamps(const std::filesystem::path& path);

/// Find the word index active at the given time, or -1 if none.
/// Returns the index of the first word whose [start, end) interval contains @p time_seconds.
[[nodiscard]] int
find_active_word_index(const WordTimestampTrack& track, double time_seconds);

} // namespace tachyon::text