#include "tachyon/timeline/time.h"

namespace tachyon {
namespace timeline {

double frame_to_seconds(const FrameRate& frame_rate, std::int64_t frame_number) noexcept {
    const double fps = frame_rate.value();
    if (fps <= 0.0) {
        return 0.0;
    }
    return static_cast<double>(frame_number) / fps;
}

FrameSample sample_frame(const FrameRate& frame_rate, std::int64_t frame_number) noexcept {
    FrameSample sample;
    sample.frame_number = frame_number;
    sample.composition_time_seconds = frame_to_seconds(frame_rate, frame_number);
    return sample;
}

double local_time_from_composition(double composition_time_seconds, double start_time_seconds) noexcept {
    return composition_time_seconds - start_time_seconds;
}

} // namespace timeline
} // namespace tachyon
