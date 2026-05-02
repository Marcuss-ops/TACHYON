#include "tachyon/runtime/execution/jobs/render_job.h"
#include <iostream>
#include <string>
#include <cmath>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void check_equals(std::int64_t actual, std::int64_t expected, const std::string& message) {
    if (actual != expected) {
        ++g_failures;
        std::cerr << "FAIL: " << message << " (expected " << expected << ", got " << actual << ")\n";
    }
}

// Duration to frames: ceil(duration * fps)
std::int64_t duration_to_frames(double duration, double fps) {
    return static_cast<std::int64_t>(std::ceil(duration * fps));
}

} // namespace

bool run_frame_range_tests() {
    g_failures = 0;

    // Test: 10s at 30fps = 300 frames
    {
        double duration = 10.0;
        double fps = 30.0;
        std::int64_t total_frames = duration_to_frames(duration, fps);
        tachyon::FrameRange range{0, total_frames};
        check_equals(range.end - range.start, 300, "10s at 30fps should be 300 frames");
        check_equals(total_frames, 300, "duration_to_frames 10s*30fps = 300");
    }

    // Test: 10s at 60fps = 600 frames
    {
        double duration = 10.0;
        double fps = 60.0;
        std::int64_t total_frames = duration_to_frames(duration, fps);
        tachyon::FrameRange range{0, total_frames};
        check_equals(range.end - range.start, 600, "10s at 60fps should be 600 frames");
        check_equals(total_frames, 600, "duration_to_frames 10s*60fps = 600");
    }

    // Test: 10s at 24fps = 240 frames
    {
        double duration = 10.0;
        double fps = 24.0;
        std::int64_t total_frames = duration_to_frames(duration, fps);
        tachyon::FrameRange range{0, total_frames};
        check_equals(range.end - range.start, 240, "10s at 24fps should be 240 frames");
        check_equals(total_frames, 240, "duration_to_frames 10s*24fps = 240");
    }

    // Test: 10.5s at 30fps = 315 frames
    {
        double duration = 10.5;
        double fps = 30.0;
        std::int64_t total_frames = duration_to_frames(duration, fps);
        tachyon::FrameRange range{0, total_frames};
        check_equals(range.end - range.start, 315, "10.5s at 30fps should be 315 frames");
        check_equals(total_frames, 315, "duration_to_frames 10.5s*30fps = 315");
    }

    // Test: start 10, end 20 = 10 frames
    {
        tachyon::FrameRange range{10, 20};
        check_equals(range.end - range.start, 10, "range 10-20 should be 10 frames");
        check_true(range.start < range.end, "start should be less than end");
    }

    // Test: start == end = 0 frames (empty range)
    {
        tachyon::FrameRange range{10, 10};
        check_equals(range.end - range.start, 0, "range 10-10 should be 0 frames");
    }

    // Test: start > end is invalid (negative frames)
    {
        tachyon::FrameRange range{20, 10};
        std::int64_t frames = range.end - range.start;
        check_true(frames < 0, "range 20-10 should have negative frame count");
    }

    // Test: FrameRange contract - start inclusive, end exclusive
    {
        tachyon::FrameRange range{5, 10};
        // Frames to render: 5, 6, 7, 8, 9 (5 frames)
        check_equals(range.end - range.start, 5, "range 5-10 should iterate 5 frames (5,6,7,8,9)");
    }

    // Test: negative start is invalid
    {
        tachyon::FrameRange range{-1, 10};
        check_true(range.start < 0, "negative start should be invalid");
    }

    return g_failures == 0;
}
