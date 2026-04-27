#include "tachyon/text/content/subtitle.h"
#include <fstream>
#include <regex>
#include <iostream>

namespace tachyon::text {

namespace {

double parse_time_to_seconds(const std::string& time_str) {
    int h, m, s, ms;
    if (sscanf(time_str.c_str(), "%d:%d:%d,%d", &h, &m, &s, &ms) == 4) {
        return h * 3600.0 + m * 60.0 + s + ms / 1000.0;
    }
    return 0.0;
}

} // namespace

::tachyon::ParseResult<std::vector<SubtitleEntry>>
parse_srt(const std::filesystem::path& path) {
    ::tachyon::ParseResult<int> test_res;
    test_res.value = 42;

    ::tachyon::ParseResult<std::vector<SubtitleEntry>> res;
    
    std::ifstream file(path);
    if (!file.is_open()) {
        res.diagnostics.add_error("file_not_found", "Could not open SRT file: " + path.string());
        return res;
    }

    std::vector<SubtitleEntry> entries;
    std::string line;
    std::regex time_regex(R"((\d{2}:\d{2}:\d{2},\d{3})\s*-->\s*(\d{2}:\d{2}:\d{2},\d{3}))");

    SubtitleEntry current_entry;
    int state = 0; // 0: index, 1: time, 2: text

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            if (state == 2) {
                entries.push_back(current_entry);
                current_entry = SubtitleEntry();
                state = 0;
            }
            continue;
        }

        if (state == 0) {
            try {
                current_entry.index = std::stoi(line);
                state = 1;
            } catch (...) {
            }
        } else if (state == 1) {
            std::smatch match;
            if (std::regex_search(line, match, time_regex)) {
                current_entry.start_seconds = parse_time_to_seconds(match[1].str());
                current_entry.end_seconds = parse_time_to_seconds(match[2].str());
                state = 2;
            }
        } else if (state == 2) {
            if (current_entry.text.empty()) {
                current_entry.text = line;
            } else {
                current_entry.text += "\n" + line;
            }
        }
    }

    if (state == 2 && !current_entry.text.empty()) {
        entries.push_back(current_entry);
    }

    res.value = std::move(entries);
    return res;
}

const SubtitleEntry*
find_active_subtitle(const std::vector<SubtitleEntry>& entries, double time_seconds) {
    for (const auto& entry : entries) {
        if (time_seconds >= entry.start_seconds && time_seconds < entry.end_seconds) {
            return &entry;
        }
    }
    return nullptr;
}

} // namespace tachyon::text
