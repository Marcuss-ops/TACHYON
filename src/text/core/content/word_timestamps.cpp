#include "tachyon/text/content/word_timestamps.h"
#include <nlohmann/json.hpp>
#include <fstream>

namespace tachyon::text {

::tachyon::ParseResult<WordTimestampTrack>
parse_word_timestamps(const std::filesystem::path& path) {
    ::tachyon::ParseResult<WordTimestampTrack> res;

    std::ifstream file(path);
    if (!file.is_open()) {
        res.diagnostics.add_error("file_not_found", "Could not open word timestamps file: " + path.string());
        return res;
    }

    try {
        nlohmann::json j;
        file >> j;

        WordTimestampTrack track;
        if (j.contains("words") && j["words"].is_array()) {
            for (const auto& item : j["words"]) {
                WordTimestamp wt;
                wt.word = item.value("word", "");
                wt.start = item.value("start", 0.0);
                wt.end = item.value("end", 0.0);
                track.words.push_back(wt);
            }
        }

        res.value = std::move(track);
        return res;
    } catch (const std::exception& e) {
        res.diagnostics.add_error("json_parse_error", "JSON parse error: " + std::string(e.what()));
        return res;
    }
}

int
find_active_word_index(const WordTimestampTrack& track, double time_seconds) {
    for (int i = 0; i < static_cast<int>(track.words.size()); ++i) {
        if (time_seconds >= track.words[i].start && time_seconds < track.words[i].end) {
            return i;
        }
    }
    return -1;
}

} // namespace tachyon::text