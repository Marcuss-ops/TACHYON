#include "tachyon/core/spec/json_report_utils.h"

namespace tachyon::spec {

json frame_range_to_json(const FrameRange& r) {
    return {{"start", r.start}, {"end", r.end}};
}

json output_destination_to_json(const OutputDestination& d) {
    return {{"path", d.path}, {"overwrite", d.overwrite}};
}

json output_profile_to_json(const OutputProfile& p) {
    return {
        {"container", p.container},
        {"video", {{"codec", p.video.codec}, {"pixel_format", p.video.pixel_format}}},
    };
}

json diagnostics_to_json(const DiagnosticBag& bag) {
    json arr = json::array();
    for (const auto& d : bag.diagnostics) {
        arr.push_back({{"code", d.code}, {"message", d.message}, {"path", d.path}});
    }
    return arr;
}

std::string diagnostics_to_text(const DiagnosticBag& bag) {
    std::string out;
    for (const auto& d : bag.diagnostics) {
        out += "[" + d.code + "] ";
        if (!d.path.empty()) out += d.path + ": ";
        out += d.message + "\n";
    }
    return out;
}
    return arr;
}

std::string diagnostics_to_text(const DiagnosticBag& bag) {
    std::string out;
    for (const auto& d : bag.diagnostics) {
        out += "[" + d.code + "] ";
        if (!d.path.empty()) out += d.path + ": ";
        out += d.message + "\n";
    }
    return out;
}

} // namespace tachyon::spec
