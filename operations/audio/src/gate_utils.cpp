#include "tachyon/audio/gate_utils.h"
#include <sstream>
#include <algorithm>

namespace tachyon::audio {

std::string GateUtils::build_gate_expr_from_ranges(
    const std::vector<std::pair<double, double>>& ranges,
    bool inverted) 
{
    if (ranges.empty()) {
        return inverted ? "0" : "1";
    }

    std::vector<std::pair<double, double>> valid_ranges;
    for (const auto& range : ranges) {
        if (range.first < range.second && range.first >= 0) {
            valid_ranges.push_back(range);
        }
    }

    if (valid_ranges.empty()) {
        return inverted ? "0" : "1";
    }

    // Sort ranges for consistency
    std::sort(valid_ranges.begin(), valid_ranges.end());

    std::stringstream ss;
    if (inverted) {
        // Active ONLY in these ranges: if(between(t, r1_s, r1_e) + between(t, r2_s, r2_e), 1, 0)
        ss << "if(";
        for (size_t i = 0; i < valid_ranges.size(); ++i) {
            if (i > 0) ss << "+";
            ss << "between(t," << valid_ranges[i].first << "," << valid_ranges[i].second << ")";
        }
        ss << ",1,0)";
    } else {
        // Muted in these ranges: if(between(t, r1_s, r1_e) + between(t, r2_s, r2_e), 0, 1)
        ss << "if(";
        for (size_t i = 0; i < valid_ranges.size(); ++i) {
            if (i > 0) ss << "+";
            ss << "between(t," << valid_ranges[i].first << "," << valid_ranges[i].second << ")";
        }
        ss << ",0,1)";
    }

    return ss.str();
}

std::string GateUtils::build_intro_only_gate_expr(double duration) {
    if (duration <= 0) return "0";
    std::stringstream ss;
    ss << "if(lt(t," << duration << "),1,0)";
    return ss.str();
}

} // namespace tachyon::audio
