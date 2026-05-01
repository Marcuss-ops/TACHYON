#include "tachyon/core/shapes/shape_modifiers.h"
#include "clipper2/clipper.h"

namespace tachyon::shapes {

void ShapeModifiers::split_cubic_bezier(const PathVertex& v1, const PathVertex& v2, double t, PathVertex& left_end, PathVertex& right_start) {
    Point2D p0 = v1.point;
    Point2D p1 = {v1.point.x + v1.out_tangent.x, v1.point.y + v1.out_tangent.y};
    Point2D p2 = {v2.point.x + v2.in_tangent.x, v2.point.y + v2.in_tangent.y};
    Point2D p3 = v2.point;

    auto lerp = [](Point2D a, Point2D b, double t) -> Point2D {
        return {static_cast<float>(a.x + (b.x - a.x) * t), static_cast<float>(a.y + (b.y - a.y) * t)};
    };

    Point2D q0 = lerp(p0, p1, t);
    Point2D q1 = lerp(p1, p2, t);
    Point2D q2 = lerp(p2, p3, t);

    Point2D r0 = lerp(q0, q1, t);
    Point2D r1 = lerp(q1, q2, t);

    Point2D s0 = lerp(r0, r1, t);

    left_end.point = s0;
    left_end.in_tangent = { (r0.x - s0.x), (r0.y - s0.y) };
    left_end.out_tangent = {0, 0};

    right_start.point = s0;
    right_start.in_tangent = {0, 0};
    right_start.out_tangent = { (r1.x - s0.x), (r1.y - s0.y) };
}

ShapePath ShapeModifiers::trim_paths(const ShapePath& path, double start, double end, double offset) {
    if (path.empty() || (std::abs(start - end) < 1e-6)) return ShapePath{};
    
    auto normalize = [](double v) {
        double res = std::fmod(v, 1.0);
        if (res < 0) res += 1.0;
        return res;
    };

    double s = normalize(start + offset);
    double e = normalize(end + offset);

    ShapePath result;
    for (const auto& subpath : path.subpaths) {
        if (subpath.vertices.size() < 2) continue;
        
        std::vector<double> segment_lengths;
        double total_length = 0.0;
        const int samples = 10;

        auto get_seg_len = [&](size_t i, size_t next_i) {
            const auto& v1 = subpath.vertices[i];
            const auto& v2 = subpath.vertices[next_i];
            Point2D p0 = v1.point;
            Point2D p1 = {v1.point.x + v1.out_tangent.x, v1.point.y + v1.out_tangent.y};
            Point2D p2 = {v2.point.x + v2.in_tangent.x, v2.point.y + v2.in_tangent.y};
            Point2D p3 = v2.point;
            double len = 0.0;
            Point2D prev = p0;
            for (int j = 1; j <= samples; ++j) {
                double t = (double)j / samples;
                double it = 1.0 - t;
                Point2D pt = {
                    static_cast<float>(it*it*it*p0.x + 3*it*it*t*p1.x + 3*it*t*t*p2.x + t*t*t*p3.x),
                    static_cast<float>(it*it*it*p0.y + 3*it*it*t*p1.y + 3*it*t*t*p2.y + t*t*t*p3.y)
                };
                len += std::sqrt(std::pow(pt.x - prev.x, 2) + std::pow(pt.y - prev.y, 2));
                prev = pt;
            }
            return len;
        };

        for (size_t i = 0; i < subpath.vertices.size() - 1; ++i) {
            double len = get_seg_len(i, i + 1);
            segment_lengths.push_back(len);
            total_length += len;
        }
        if (subpath.closed) {
            double len = get_seg_len(subpath.vertices.size() - 1, 0);
            segment_lengths.push_back(len);
            total_length += len;
        }

        if (total_length < 1e-6) continue;

        auto extract_range = [&](double t_start, double t_end) {
            ShapeSubpath sub;
            double current_l = 0.0;
            size_t num_segs = segment_lengths.size();
            
            for (size_t i = 0; i < num_segs; ++i) {
                double l0 = current_l / total_length;
                double l1 = (current_l + segment_lengths[i]) / total_length;
                
                if (l1 < t_start) {
                    current_l += segment_lengths[i];
                    continue;
                }
                if (l0 > t_end) break;

                double local_s = std::max(0.0, (t_start - l0) / (l1 - l0));
                double local_e = std::min(1.0, (t_end - l0) / (l1 - l0));

                if (local_e > local_s) {
                    const auto& v1 = subpath.vertices[i % subpath.vertices.size()];
                    const auto& v2 = subpath.vertices[(i + 1) % subpath.vertices.size()];
                    
                    PathVertex start_v = v1;
                    PathVertex end_v = v2;
                    
                    if (local_e < 1.0) {
                        PathVertex dummy;
                        split_cubic_bezier(v1, end_v, local_e, end_v, dummy);
                    }
                    if (local_s > 0.0) {
                        PathVertex dummy;
                        split_cubic_bezier(start_v, end_v, local_s, dummy, start_v);
                    }
                    
                    if (sub.vertices.empty()) {
                        sub.vertices.push_back(start_v);
                    } else {
                        sub.vertices.back().out_tangent = start_v.out_tangent;
                    }
                    sub.vertices.push_back(end_v);
                }
                current_l += segment_lengths[i];
            }
            return sub;
        };

        if (s <= e) {
            ShapeSubpath res_sub = extract_range(s, e);
            if (!res_sub.vertices.empty()) result.subpaths.push_back(res_sub);
        } else {
            ShapeSubpath res1 = extract_range(s, 1.0);
            ShapeSubpath res2 = extract_range(0.0, e);
            if (!res1.vertices.empty()) result.subpaths.push_back(res1);
            if (!res2.vertices.empty()) result.subpaths.push_back(res2);
        }
    }
    return result;
}

ShapePath ShapeModifiers::repeater(const ShapePath& path, int copies, const RepeaterTransform& transform) {
    ShapePath result;
    if (copies <= 0 || path.empty()) return result;

    result.subpaths.reserve(path.subpaths.size() * copies);

    for (int i = 0; i < copies; ++i) {
        double angle_rad = (transform.rotation * i) * 3.14159265358979323846 / 180.0;
        double cos_a = std::cos(angle_rad);
        double sin_a = std::sin(angle_rad);
        
        double tx = transform.position.x * i;
        double ty = transform.position.y * i;
        
        double sx = std::pow(transform.scale.x, i);
        double sy = std::pow(transform.scale.y, i);

        for (const auto& subpath : path.subpaths) {
            ShapeSubpath copy_subpath;
            copy_subpath.closed = subpath.closed;
            copy_subpath.vertices.reserve(subpath.vertices.size());

            for (const auto& v : subpath.vertices) {
                PathVertex new_v = v;
                auto apply_transform = [&](Point2D pt) -> Point2D {
                    pt.x -= transform.anchor_point.x;
                    pt.y -= transform.anchor_point.y;
                    pt.x *= static_cast<float>(sx);
                    pt.y *= static_cast<float>(sy);
                    double rx = pt.x * cos_a - pt.y * sin_a;
                    double ry = pt.x * sin_a + pt.y * cos_a;
                    return {static_cast<float>(rx + tx + transform.anchor_point.x), static_cast<float>(ry + ty + transform.anchor_point.y)};
                };

                new_v.point = apply_transform(v.point);
                
                auto apply_vector_transform = [&](Point2D vec) -> Point2D {
                    vec.x *= static_cast<float>(sx); vec.y *= static_cast<float>(sy);
                    return {static_cast<float>(vec.x * cos_a - vec.y * sin_a), static_cast<float>(vec.x * sin_a + vec.y * cos_a)};
                };

                new_v.in_tangent = apply_vector_transform(v.in_tangent);
                new_v.out_tangent = apply_vector_transform(v.out_tangent);
                
                copy_subpath.vertices.push_back(new_v);
            }
            result.subpaths.push_back(copy_subpath);
        }
    }

    return result;
}

ShapePath ShapeModifiers::offset_path(const ShapePath& path, double amount) {
    if (path.empty() || amount == 0.0) return path;

    ShapePath result;
    for (const auto& subpath : path.subpaths) {
        if (subpath.vertices.size() < 2) continue;
        ShapeSubpath offset_sub;
        offset_sub.closed = subpath.closed;

        for (std::size_t i = 0; i < subpath.vertices.size(); ++i) {
            const auto& curr = subpath.vertices[i];
            const auto& prev = subpath.vertices[(i == 0) ? (subpath.closed ? subpath.vertices.size() - 1 : 0) : i - 1];
            const auto& next = subpath.vertices[(i == subpath.vertices.size() - 1) ? (subpath.closed ? 0 : i) : i + 1];

            Point2D t_in = {curr.point.x - prev.point.x, curr.point.y - prev.point.y};
            Point2D t_out = {next.point.x - curr.point.x, next.point.y - curr.point.y};
            
            if (std::abs(t_in.x) < 1e-6 && std::abs(t_in.y) < 1e-6) t_in = t_out;
            if (std::abs(t_out.x) < 1e-6 && std::abs(t_out.y) < 1e-6) t_out = t_in;

            Point2D normal = {-(t_in.y + t_out.y), (t_in.x + t_out.x)};
            double len = std::sqrt(normal.x * normal.x + normal.y * normal.y);
            if (len > 1e-6) { normal.x /= static_cast<float>(len); normal.y /= static_cast<float>(len); }

            PathVertex new_v = curr;
            new_v.point.x += static_cast<float>(normal.x * amount);
            new_v.point.y += static_cast<float>(normal.y * amount);
            offset_sub.vertices.push_back(new_v);
        }
        result.subpaths.push_back(offset_sub);
    }
    return result;
}

ShapePath ShapeModifiers::merge_paths(const ShapePath& a, const ShapePath& b, int mode) {
    using namespace Clipper2Lib;

    auto flatten_subpath = [](const ShapeSubpath& sub, PathsD& paths) {
        PathD path;
        for (size_t i = 0; i < sub.vertices.size(); ++i) {
            const auto& v1 = sub.vertices[i];
            const auto& v2 = sub.vertices[(i + 1) % sub.vertices.size()];
            
            if (!sub.closed && i == sub.vertices.size() - 1) {
                path.push_back(PointD(v1.point.x, v1.point.y));
                break;
            }

            auto p0 = v1.point;
            auto p1 = Point2D{v1.point.x + v1.out_tangent.x, v1.point.y + v1.out_tangent.y};
            auto p2 = Point2D{v2.point.x + v2.in_tangent.x, v2.point.y + v2.in_tangent.y};
            auto p3 = v2.point;

            if (v1.out_tangent.x == 0 && v1.out_tangent.y == 0 && v2.in_tangent.x == 0 && v2.in_tangent.y == 0) {
                path.push_back(PointD(p0.x, p0.y));
            } else {
                const int steps = 24;
                for (int s = 0; s < steps; ++s) {
                    double t = (double)s / steps;
                    double inv_t = 1.0 - t;
                    double b0 = inv_t * inv_t * inv_t;
                    double b1 = 3.0 * inv_t * inv_t * t;
                    double b2 = 3.0 * inv_t * t * t;
                    double b3 = t * t * t;
                    path.push_back(PointD(
                        b0 * p0.x + b1 * p1.x + b2 * p2.x + b3 * p3.x,
                        b0 * p0.y + b1 * p1.y + b2 * p2.y + b3 * p3.y
                    ));
                }
            }
        }
        paths.push_back(path);
    };

    auto to_clipper_paths = [&](const ShapePath& sp) -> PathsD {
        PathsD paths;
        for (const auto& sub : sp.subpaths) {
            flatten_subpath(sub, paths);
        }
        return paths;
    };

    auto from_clipper_paths = [](const PathsD& cp) -> ShapePath {
        ShapePath sp;
        for (const auto& c_path : cp) {
            ShapeSubpath sub;
            sub.closed = true;
            for (const auto& pt : c_path) {
                sub.vertices.push_back({
                    {static_cast<float>(pt.x), static_cast<float>(pt.y)},
                    {0.0f, 0.0f},
                    {0.0f, 0.0f}
                });
            }
            sp.subpaths.push_back(sub);
        }
        return sp;
    };

    PathsD subj = to_clipper_paths(a);
    PathsD clip = to_clipper_paths(b);
    PathsD solution;

    ClipType clip_type;
    switch (mode) {
        case 0: clip_type = ClipType::Union; break;
        case 1: clip_type = ClipType::Difference; break;
        case 2: clip_type = ClipType::Intersection; break;
        case 3: clip_type = ClipType::Xor; break;
        default: clip_type = ClipType::Union; break;
    }

    solution = BooleanOp(clip_type, FillRule::NonZero, subj, clip);

    return from_clipper_paths(solution);
}

} // namespace tachyon::shapes
