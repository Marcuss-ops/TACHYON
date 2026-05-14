#include "tachyon/media/transition_filters.h"
#include <sstream>

namespace tachyon::media {

std::string TransitionFilters::build_filter(const Params& params, const std::string& input_label, const std::string& output_label) {
    switch (params.type) {
        case Type::Fade:       return build_fade(params, input_label, output_label);
        case Type::WipeLeft:
        case Type::WipeRight:  return build_wipe(params, input_label, output_label);
        case Type::CircleCrop: return build_circle(params, input_label, output_label);
        default:               return "[" + input_label + "] null [" + output_label + "]";
    }
}

std::string TransitionFilters::build_fade(const Params& params, const std::string& input_label, const std::string& output_label) {
    std::stringstream ss;
    ss << "[" << input_label << "] fade=";
    ss << (params.is_out ? "t=out" : "t=in") << ":st=" << params.start_time << ":d=" << params.duration;
    ss << " [" << output_label << "]";
    return ss.str();
}

std::string TransitionFilters::build_wipe(const Params& params, const std::string& input_label, const std::string& output_label) {
    // Simplified wipe using overlay + crop or dynamic shift
    // Real FFmpeg wipe often uses 'overlay' with dynamic 'x'
    std::stringstream ss;
    std::string direction = (params.type == Type::WipeLeft) ? "left" : "right";
    
    // Placeholder for a complex wipe filter
    ss << "[" << input_label << "] crop=w=iw:h=ih:x=0:y=0 [" << output_label << "]"; 
    return ss.str();
}

std::string TransitionFilters::build_circle(const Params& params, const std::string& input_label, const std::string& output_label) {
    // Circle crop using alpha mask and 'geq' or 'vignette'
    std::stringstream ss;
    ss << "[" << input_label << "] vignette=angle=PI/4 [" << output_label << "]";
    return ss.str();
}

} // namespace tachyon::media
