#include "tachyon/text/content/subtitle.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

namespace tachyon::text {

// ---------------------------------------------------------------------------
// SRT timing parser
// ---------------------------------------------------------------------------

// "HH:MM:SS,mmm" â†’ seconds as double
// Returns -1 on parse failure.
static double parse_srt_time(const std::string& token) {
    // token: "HH:MM:SS,mmm"
    if (token.size() < 12) {
        return -1.0;
    }
    try {
        const int hh  = std::stoi(token.substr(0, 2));
        const int mm  = std::stoi(token.substr(3, 2));
        const int ss  = std::stoi(token.substr(6, 2));
        const int ms  = std::stoi(token.substr(9, 3));
        return static_cast<double>(hh) * 3600.0
             + static_cast<double>(mm) * 60.0
             + static_cast<double>(ss)
             + static_cast<double>(ms) / 1000.0;
    } catch (...) {
        return -1.0;
    }
}

// Parse "HH:MM:SS,mmm --> HH:MM:SS,mmm" timing line.
static bool parse_timing_line(const std::string& line, double& start, double& end) {
    // Find " --> "
    const std::string arrow = " --> ";
    const auto arrow_pos = line.find(arrow);
    if (arrow_pos == std::string::npos) {
        return false;
    }
    const std::string start_token = line.substr(0, arrow_pos);
    const std::string end_token   = line.substr(arrow_pos + arrow.size());

    // Strip trailing whitespace from end_token (some files add position info)
    std::string clean_end = end_token;
    const auto end_space = clean_end.find(' ');
    if (end_space != std::string::npos) {
        clean_end = clean_end.substr(0, end_space);
    }

    start = parse_srt_time(start_token);
    end   = parse_srt_time(clean_end);
    return (start >= 0.0 && end >= 0.0);
}

// ---------------------------------------------------------------------------
// SRT state machine
// ---------------------------------------------------------------------------

ParseResult<std::vector<SubtitleEntry>> parse_srt(const std::filesystem::path& path) {
    ParseResult<std::vector<SubtitleEntry>> result;

    std::ifstream file(path);
    if (!file.is_open()) {
        result.diagnostics.add_error(
            "subtitle.open_failed",
            "Failed to open SRT file: " + path.string());
        return result;
    }

    enum class State { Index, Timing, Text, Blank };

    std::vector<SubtitleEntry> entries;
    State state = State::Blank;
    SubtitleEntry current;
    std::string line;

    const auto flush = [&]() {
        if (!current.text.empty()) {
            entries.push_back(std::move(current));
            current = {};
        }
    };

    while (std::getline(file, line)) {
        // Strip Windows \r
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        switch (state) {
        case State::Blank:
        case State::Index:
            // Expect an integer index â€” if it looks like one, advance
            if (!line.empty()) {
                bool all_digits = true;
                for (char ch : line) {
                    if (ch < '0' || ch > '9') { all_digits = false; break; }
                }
                if (all_digits) {
                    current.index = std::stoi(line);
                    state = State::Timing;
                }
                // Non-integer non-blank line in Blank state â†’ try timing directly
                else {
                    double s = 0.0;
                    double e = 0.0;
                    if (parse_timing_line(line, s, e)) {
                        current.start_seconds = s;
                        current.end_seconds   = e;
                        state = State::Text;
                    }
                }
            }
            break;

        case State::Timing: {
            double s = 0.0;
            double e = 0.0;
            if (parse_timing_line(line, s, e)) {
                current.start_seconds = s;
                current.end_seconds   = e;
                state = State::Text;
            } else {
                // Unexpected line â€” reset
                current = {};
                state = State::Index;
            }
            break;
        }

        case State::Text:
            if (line.empty()) {
                flush();
                state = State::Index;
            } else {
                if (!current.text.empty()) {
                    current.text += '\n';
                }
                current.text += line;
            }
            break;
        }
    }

    // Flush last entry (file may not end with a blank line)
    flush();

    result.value = std::move(entries);
    return result;
}

const SubtitleEntry* find_active_subtitle(
    const std::vector<SubtitleEntry>& entries,
    double time_seconds) {

    for (const auto& entry : entries) {
        if (time_seconds >= entry.start_seconds && time_seconds < entry.end_seconds) {
            return &entry;
        }
    }
    return nullptr;
}

} // namespace tachyon::text
