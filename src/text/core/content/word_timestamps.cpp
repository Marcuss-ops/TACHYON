#include "tachyon/text/content/word_timestamps.h"
#include <fstream>
#include <string>
#include <vector>

namespace tachyon::text {

ParseResult<WordTimestampTrack>
parse_word_timestamps(const std::filesystem::path& path) {
    ParseResult<WordTimestampTrack> result;
    (void)path;
    result.diagnostics.add_error("word_timestamps.json_removed", "JSON word timestamps are no longer supported.");
    return result;
}

int find_active_word_index(const WordTimestampTrack& track, double time_seconds) {
    for (size_t i = 0; i < track.words.size(); ++i) {
        const auto& wt = track.words[i];
        if (time_seconds >= wt.start && time_seconds < wt.end) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

} // namespace tachyon::text