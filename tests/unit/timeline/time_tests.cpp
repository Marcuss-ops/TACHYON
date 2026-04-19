#include "tachyon/timeline/time.h"

#include <cmath>
#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

} // namespace

bool run_timeline_tests() {
    g_failures = 0;

    {
        tachyon::FrameRate frame_rate{30, 1};
        const auto sample = tachyon::timeline::sample_frame(frame_rate, 15);
        check_true(sample.frame_number == 15, "sample_frame should preserve the requested frame number");
        check_true(std::abs(sample.composition_time_seconds - 0.5) < 1e-9, "frame 15 at 30 fps should sample to 0.5 seconds");
    }

    {
        const double local = tachyon::timeline::local_time_from_composition(2.0, 1.25);
        check_true(std::abs(local - 0.75) < 1e-9, "local time should subtract layer start time from composition time");
    }

    return g_failures == 0;
}
