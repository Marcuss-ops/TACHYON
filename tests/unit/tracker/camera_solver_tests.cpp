#include "tachyon/tracker/camera_solver.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void check_close(float value, float expected, float epsilon, const std::string& message) {
    if (std::abs(value - expected) > epsilon) {
        ++g_failures;
        std::cerr << "FAIL: " << message << " expected=" << expected << " value=" << value << '\n';
    }
}

tachyon::tracker::Point2f project_point(
    const tachyon::tracker::CameraSolver::Config& cfg,
    const tachyon::tracker::Point2f& world_xy,
    float depth_z,
    float camera_tx) {
    const float x_cam = world_xy.x + camera_tx;
    const float y_cam = world_xy.y;
    const float z_cam = depth_z;
    const float u = x_cam / z_cam;
    const float v = y_cam / z_cam;
    const float half_w = static_cast<float>(cfg.frame_width) * 0.5f;
    const float half_h = static_cast<float>(cfg.frame_height) * 0.5f;
    return {
        half_w + u * half_w,
        half_h + v * half_h
    };
}

} // namespace

bool run_camera_solver_tests() {
    g_failures = 0;

    tachyon::tracker::CameraSolver solver;
    tachyon::tracker::CameraSolver::Config cfg;
    cfg.frame_width = 1280;
    cfg.frame_height = 720;
    cfg.sensor_width_mm = 36.0f;
    cfg.initial_focal_length_mm = 35.0f;

    std::vector<tachyon::tracker::Track> tracks;
    const std::vector<tachyon::tracker::Point2f> world_points = {
        {-0.8f, -0.6f},
        {-0.5f,  0.4f},
        {-0.2f, -0.3f},
        { 0.1f,  0.6f},
        { 0.3f, -0.5f},
        { 0.6f,  0.2f},
        { 0.8f, -0.1f},
        { 0.4f,  0.7f},
        {-0.7f,  0.1f},
        { 0.2f, -0.8f}
    };

    for (std::size_t i = 0; i < world_points.size(); ++i) {
        tachyon::tracker::Track track;
        track.name = "track_" + std::to_string(i);
        const float depth = 5.0f + 0.2f * static_cast<float>(i);
        const auto p0 = project_point(cfg, world_points[i], depth, 0.0f);
        const auto p1 = project_point(cfg, world_points[i], depth, -0.2f);
        track.samples.push_back({0.0f, p0, 1.0f});
        track.samples.push_back({1.0f, p1, 1.0f});
        tracks.push_back(track);
    }

    const auto solved = solver.solve(tracks, cfg);
    check_true(solved.keyframes.size() == 2, "solver should emit two keyframes for two-frame input");
    if (solved.keyframes.size() == 2) {
        check_close(solved.keyframes[0].time, 0.0f, 1.0e-6f, "first keyframe time");
        check_close(solved.keyframes[1].time, 1.0f, 1.0e-6f, "second keyframe time");
        check_close(solved.keyframes[0].position.x, 0.0f, 1.0e-3f, "first keyframe x");
        check_close(solved.keyframes[0].position.y, 0.0f, 1.0e-3f, "first keyframe y");
        check_close(solved.keyframes[0].position.z, 0.0f, 1.0e-3f, "first keyframe z");
        check_true(solved.keyframes[1].position.x > 0.0f, "camera should move forward in positive X for a rightward source shift");
        check_true(std::abs(solved.keyframes[1].rotation.w) > 0.5f, "rotation should remain close to identity for pure translation");
    }

    return g_failures == 0;
}

