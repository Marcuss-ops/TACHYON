#include "tachyon/renderer2d/path/contour/contour_processing.h"
#include "tachyon/renderer2d/path/flattening/path_flattening.h"
#include <cmath>

namespace tachyon::renderer2d {

std::vector<Contour> ContourProcessor::build_contours(const PathGeometry& path, float tolerance) {
    constexpr float kCloseEpsilon = 0.01f;
    std::vector<Contour> contours;
    Contour current;
    math::Vector2 current_point{};
    bool has_current_point = false;
    bool contour_open = false;

    auto close_contour = [&]() {
        if (contour_open && current.points.size() >= 2U) {
            const math::Vector2& first = current.points.front();
            const math::Vector2& last = current.points.back();
            if (std::abs(first.x - last.x) > kCloseEpsilon || std::abs(first.y - last.y) > kCloseEpsilon) {
                current.points.push_back(current.points.front());
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
                current.points.push_back(command.p0);
                current_point = command.p0;
                has_current_point = true;
                contour_open = true;
                break;
            case PathVerb::LineTo:
                if (!has_current_point) {
                    current.points.push_back(command.p0);
                    current_point = command.p0;
                    has_current_point = true;
                    contour_open = true;
                } else {
                    current.points.push_back(command.p0);
                    current_point = command.p0;
                }
                break;
            case PathVerb::CubicTo:
                if (!has_current_point) {
                    current.points.push_back(command.p0);
                    current_point = command.p0;
                    has_current_point = true;
                    contour_open = true;
                }
                PathFlattener::flatten_cubic(current_point, command.p0, command.p1, command.p2, current.points, tolerance);
                current_point = command.p2;
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

} // namespace tachyon::renderer2d
