#pragma once

#include <cmath>
#include <algorithm>

namespace tachyon::animation {

/**
 * @brief Solves the 1D cubic Bezier curve for t given x.
 * 
 * Used for temporal Bezier interpolation where the X axis is Time and Y is Value Progress.
 * Given normalized time X in [0, 1] and control points (x1, y1), (x2, y2),
 * this finds the parametric t that yields X, then evaluates the Y component for that t.
 */
class BezierSolver {
public:
    static double solve(double x, double x1, double y1, double x2, double y2) {
        if (x <= 0.0) return 0.0;
        if (x >= 1.0) return 1.0;

        // Try Newton-Raphson first
        double t = x; // Initial guess
        for (int i = 0; i < 8; ++i) {
            double current_x = evaluate_bezier(t, x1, x2) - x;
            if (std::abs(current_x) < 1e-6) {
                return evaluate_bezier(t, y1, y2);
            }
            double slope = evaluate_derivative(t, x1, x2);
            if (std::abs(slope) < 1e-6) break;
            t -= current_x / slope;
        }

        // Fallback to bisection
        double t0 = 0.0;
        double t1 = 1.0;
        t = x;
        for (int i = 0; i < 20; ++i) {
            double current_x = evaluate_bezier(t, x1, x2);
            if (std::abs(current_x - x) < 1e-6) break;
            if (x > current_x) {
                t0 = t;
            } else {
                t1 = t;
            }
            t = (t1 + t0) * 0.5;
        }

        return evaluate_bezier(t, y1, y2);
    }

private:
    static double evaluate_bezier(double t, double p1, double p2) {
        double u = 1.0 - t;
        return 3.0 * u * u * t * p1 + 3.0 * u * t * t * p2 + t * t * t;
    }

    static double evaluate_derivative(double t, double p1, double p2) {
        double u = 1.0 - t;
        return 3.0 * u * u * p1 + 6.0 * u * t * (p2 - p1) + 3.0 * t * t * (1.0 - p2);
    }
};

} // namespace tachyon::animation
