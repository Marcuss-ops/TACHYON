#include "tachyon/core/media/transition_filters.h"
#include <sstream>

namespace tachyon::core::media {

std::string TransitionFilters::generate_fade_in(double duration) {
    std::stringstream ss;
    ss << "fade=t=in:st=0:d=" << duration;
    return ss.str();
}

std::string TransitionFilters::generate_fade_out(double start_time, double duration) {
    std::stringstream ss;
    ss << "fade=t=out:st=" << start_time << ":d=" << duration;
    return ss.str();
}

} // namespace tachyon::core::media
