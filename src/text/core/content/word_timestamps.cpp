#include "tachyon/text/content/word_timestamps.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace tachyon::text {

ParseResult<WordTimestampTrack>
parse_word_timestamps(const std::filesystem::path& path) {
    ParseResult<WordTimestampTrack> result;

    std::ifstream file(path);
    if (!file.is_open()) {
        result.diagnostics.add_error(
            "word_timestamps.open_failed",
            "Failed to open word timestamps file: " + path.string());
        return result;
    }

    try {
        json j;
        file >> j;

        if (!j.is_object()) {
            result.diagnostics.add_error(
                "word_timestamps.root_invalid",
                "Word timestamps JSON root must be an object");
            return result;
        }

        if (!j.contains("words") || !j["words"].is_array()) {
            result.diagnostics.add_error(
                "word_timestamps.missing_words_array",
                "Word timestamps JSON must contain a 'words' array");
            return result;
        }

        const auto& words_json = j["words"];
        WordTimestampTrack track;
        track.words.reserve(words_json.size());

        for (size_t i = 0; i < words_json.size(); ++i) {
            const auto& word_json = words_json[i];
            if (!word_json.is_object()) {
                result.diagnostics.add_warning(
                    "word_timestamps.invalid_entry",
                    "Word entry at index " + std::to_string(i) + " is not an object, skipping");
                continue;
            }

            WordTimestamp wt;

            // Parse word text
            if (!word_json.contains("word") || !word_json["word"].is_string()) {
                result.diagnostics.add_warning(
                    "word_timestamps.missing_word_text",
                    "Word entry at index " + std::to_string(i) + " missing 'word' string, skipping");
                continue;
            }
            wt.word = word_json["word"].get<std::string>();

            // Parse start time
            if (!word_json.contains("start") || !word_json["start"].is_number()) {
                result.diagnostics.add_warning(
                    "word_timestamps.missing_start_time",
                    "Word '" + wt.word + "' missing 'start' number, skipping");
                continue;
            }
            wt.start = word_json["start"].get<double>();

            // Parse end time
            if (!word_json.contains("end") || !word_json["end"].is_number()) {
                result.diagnostics.add_warning(
                    "word_timestamps.missing_end_time",
                    "Word '" + wt.word + "' missing 'end' number, skipping");
                continue;
            }
            wt.end = word_json["end"].get<double>();

            // Validate time range
            if (wt.end < wt.start) {
                result.diagnostics.add_warning(
                    "word_timestamps.invalid_time_range",
                    "Word '" + wt.word + "' has end < start, skipping");
                continue;
            }

            track.words.push_back(std::move(wt));
        }

        result.value = std::move(track);
        return result;

    } catch (const json::parse_error& e) {
        result.diagnostics.add_error(
            "word_timestamps.parse_error",
            "Failed to parse JSON: " + std::string(e.what()));
        return result;
    } catch (const std::exception& e) {
        result.diagnostics.add_error(
            "word_timestamps.processing_error",
            "Error processing word timestamps: " + std::string(e.what()));
        return result;
    }
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