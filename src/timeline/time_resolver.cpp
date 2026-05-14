#include "tachyon/timeline/evaluator.h"

#include <algorithm>

namespace tachyon::timeline {

double frame_to_seconds(std::int64_t frame_number, const FrameRate& frame_rate) {
    const double fps = frame_rate.value();
    if (fps <= 0.0) {
        return 0.0;
    }
    return static_cast<double>(frame_number) / fps;
}

bool is_layer_visible_at_time(const LayerSpec& layer, double composition_time_seconds, double composition_duration_seconds) {
    if (!layer.enabled) {
        return false;
    }

    const double effective_in = std::max(0.0, layer.timing.start);
    const double out_point = layer.timing.start + layer.timing.duration;
    const double effective_out = (out_point > effective_in) ? out_point : composition_duration_seconds;
    return composition_time_seconds >= effective_in && composition_time_seconds < effective_out;
}

} // namespace tachyon::timeline
