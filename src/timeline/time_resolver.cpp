#include "tachyon/timeline/evaluator.h"

#include <algorithm>

namespace tachyon::timeline {

bool is_layer_visible_at_time(const LayerSpec& layer, double composition_time_seconds, double composition_duration_seconds) {
    if (!layer.enabled) {
        return false;
    }

    const double effective_in = std::max(0.0, layer.in_point);
    const double effective_out = (layer.out_point > effective_in) ? layer.out_point : composition_duration_seconds;
    return composition_time_seconds >= effective_in && composition_time_seconds < effective_out;
}

} // namespace tachyon::timeline
