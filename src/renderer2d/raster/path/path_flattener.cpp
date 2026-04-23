#include "tachyon/renderer2d/raster/path/path_flattener.h"
#include <cmath>
#include <algorithm>

namespace tachyon::renderer2d {

void flatten_cubic(
    const math::Vector2& p0,
    const math::Vector2& p1,
    const math::Vector2& p2,
    const math::Vector2& p3,
    std::vector<ContourPoint>& out,
    float tolerance,
    std::uint32_t depth) {

    const math::Vector2 chord = p3 - p0;
    const float chord_length = chord.length();
    const float distance_1 = chord_length > 0.0f ? std::abs((p1.x - p0.x) * chord.y - (p1.y - p0.y) * chord.x) / chord_length : (p1 - p0).length();
    const float distance_2 = chord_length > 0.0f ? std::abs((p2.x - p0.x) * chord.y - (p2.y - p0.y) * chord.x) / chord_length : (p2 - p0).length();

    if ((std::max(distance_1, distance_2) <= tolerance) || depth >= 12U) {
        out.push_back({p3, 0.0f, 0.0f});
        return;
    }

    const math::Vector2 p01 = (p0 + p1) * 0.5f;
    const math::Vector2 p12 = (p1 + p2) * 0.5f;
    const math::Vector2 p23 = (p2 + p3) * 0.5f;
    const math::Vector2 p012 = (p01 + p12) * 0.5f;
    const math::Vector2 p123 = (p12 + p23) * 0.5f;
    const math::Vector2 p0123 = (p012 + p123) * 0.5f;

    flatten_cubic(p0, p01, p012, p0123, out, tolerance, depth + 1U);
    flatten_cubic(p0123, p123, p23, p3, out, tolerance, depth + 1U);
}

std::vector<Contour> build_contours(const PathGeometry& path) {
    constexpr float kCloseEpsilon = 0.01f;
    std::vector<Contour> contours;
    Contour current;
    math::Vector2 current_point{};
    bool has_current_point = false;
    bool contour_open = false;
    float feather_inner = 0.0f;
    float feather_outer = 0.0f;

    auto close_contour = [&]() {
        if (contour_open && current.points.size() >= 2U) {
            const auto& first = current.points.front();
            const auto& last = current.points.back();
            if (std::abs(first.point.x - last.point.x) > kCloseEpsilon || std::abs(first.point.y - last.point.y) > kCloseEpsilon) {
                current.points.push_back(first);
            }
        }
        if (!current.points.empty()) {
            contours.push_back(std::move(current));
        }
        current = {};
        contour_open = false;
        has_current_point = false;
    };

    for (const PathCommand& command : path.commands) {
        switch (command.verb) {
            case PathVerb::MoveTo:
                close_contour();
                current.points.push_back({command.p0, command.feather_inner, command.feather_outer});
                current_point = command.p0;
                feather_inner = command.feather_inner;
                feather_outer = command.feather_outer;
                has_current_point = true;
                contour_open = true;
                break;
            case PathVerb::LineTo:
                if (!has_current_point) {
                    current.points.push_back({command.p0, feather_inner, feather_outer});
                    current_point = command.p0;
                    has_current_point = true;
                    contour_open = true;
                } else {
                    current.points.push_back({command.p0, command.feather_inner, command.feather_outer});
                    current_point = command.p0;
                    feather_inner = command.feather_inner;
                    feather_outer = command.feather_outer;
                }
                break;
            case PathVerb::CubicTo:
                if (!has_current_point) {
                    current.points.push_back({command.p0, feather_inner, feather_outer});
                    current_point = command.p0;
                    has_current_point = true;
                    contour_open = true;
                }
                flatten_cubic(current_point, command.p0, command.p1, command.p2, current.points, 0.35f);
                current_point = command.p2;
                feather_inner = command.feather_inner;
                feather_outer = command.feather_outer;
                break;
            case PathVerb::Close:
                close_contour();
                break;
        }
    }

    if (!current.points.empty()) {
        close_contour();
    }

    return contours;
}

float segment_length(const math::Vector2& p0, const math::Vector2& p1, const math::Vector2& p2, const math::Vector2& p3, PathVerb verb) {
    if (verb == PathVerb::LineTo) {
        return (p1 - p0).length();
    } else if (verb == PathVerb::CubicTo) {
        std::vector<ContourPoint> flattened;
        flattened.push_back({p0, 0.0f, 0.0f});
        flatten_cubic(p0, p1, p2, p3, flattened, 0.5f);
        float len = 0.0f;
        for (std::size_t i = 0; i + 1 < flattened.size(); ++i) {
            len += (flattened[i + 1].point - flattened[i].point).length();
        }
        return len;
    }
    return 0.0f;
}

} // namespace tachyon::renderer2d
