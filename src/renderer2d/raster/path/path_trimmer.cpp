#include "tachyon/renderer2d/raster/path/path_trimmer.h"
#include "tachyon/renderer2d/raster/path/path_flattener.h"
#include <cmath>
#include <algorithm>

namespace tachyon::renderer2d {

namespace {

void split_cubic(const math::Vector2& p0, const math::Vector2& p1, const math::Vector2& p2, const math::Vector2& p3, float t, math::Vector2* left, math::Vector2* right) {
    const math::Vector2 p01 = p0 * (1.0f - t) + p1 * t;
    const math::Vector2 p12 = p1 * (1.0f - t) + p2 * t;
    const math::Vector2 p23 = p2 * (1.0f - t) + p3 * t;
    const math::Vector2 p012 = p01 * (1.0f - t) + p12 * t;
    const math::Vector2 p123 = p12 * (1.0f - t) + p23 * t;
    const math::Vector2 p0123 = p012 * (1.0f - t) + p123 * t;
    
    if (left) {
        left[0] = p0;
        left[1] = p01;
        left[2] = p012;
        left[3] = p0123;
    }
    if (right) {
        right[0] = p0123;
        right[1] = p123;
        right[2] = p23;
        right[3] = p3;
    }
}

void extract_sub_cubic(const math::Vector2& p0, const math::Vector2& p1, const math::Vector2& p2, const math::Vector2& p3, float t1, float t2, math::Vector2* out) {
    if (t1 <= 0.0f && t2 >= 1.0f) {
        out[0] = p0; out[1] = p1; out[2] = p2; out[3] = p3;
        return;
    }
    if (t1 >= 1.0f || t2 <= 0.0f || t1 >= t2) {
        out[0] = out[1] = out[2] = out[3] = (t1 >= 1.0f ? p3 : p0);
        return;
    }

    math::Vector2 right[4];
    split_cubic(p0, p1, p2, p3, t1, nullptr, right);
    
    const float nt2 = (t2 - t1) / (1.0f - t1);
    split_cubic(right[0], right[1], right[2], right[3], nt2, out, nullptr);
}

} // namespace

PathGeometry trim_path(const PathGeometry& path, float start, float end, float offset) {
    if (path.commands.empty()) return path;
    
    auto wrap = [](float v) {
        v = std::fmod(v, 1.0f);
        if (v < 0.0f) v += 1.0f;
        return v;
    };
    
    const float t_start = wrap(start + offset);
    const float t_end = wrap(end + offset);
    
    struct Segment {
        PathVerb verb;
        math::Vector2 p[4]; 
        float length;
        float cum_length;
    };
    
    std::vector<Segment> segments;
    float total_length = 0.0f;
    math::Vector2 current_point{0, 0};
    math::Vector2 move_to_point{0, 0};
    
    for (const auto& cmd : path.commands) {
        if (cmd.verb == PathVerb::MoveTo) {
            current_point = cmd.p0;
            move_to_point = cmd.p0;
        } else if (cmd.verb == PathVerb::LineTo) {
            float len = (cmd.p0 - current_point).length();
            segments.push_back({PathVerb::LineTo, {current_point, cmd.p0, {}, {}}, len, 0.0f});
            total_length += len;
            current_point = cmd.p0;
        } else if (cmd.verb == PathVerb::CubicTo) {
            float len = segment_length(current_point, cmd.p0, cmd.p1, cmd.p2, PathVerb::CubicTo);
            segments.push_back({PathVerb::CubicTo, {current_point, cmd.p0, cmd.p1, cmd.p2}, len, 0.0f});
            total_length += len;
            current_point = cmd.p2;
        } else if (cmd.verb == PathVerb::Close) {
             float len = (move_to_point - current_point).length();
             if (len > 1e-6f) {
                 segments.push_back({PathVerb::LineTo, {current_point, move_to_point, {}, {}}, len, 0.0f});
                 total_length += len;
             }
             current_point = move_to_point;
        }
    }
    
    if (total_length < 1e-6f) return path;
    
    float acc = 0.0f;
    for (auto& seg : segments) {
        seg.cum_length = acc;
        acc += seg.length;
    }
    
    auto collect_range = [&](float s, float e, PathGeometry& out) {
        const float start_len = s * total_length;
        const float end_len = e * total_length;
        
        bool first_move = true;
        
        for (const auto& seg : segments) {
            const float s_start = seg.cum_length;
            const float s_end = seg.cum_length + seg.length;
            
            if (s_end < start_len || s_start > end_len) continue;
            
            const float local_s = std::clamp((start_len - s_start) / seg.length, 0.0f, 1.0f);
            const float local_e = std::clamp((end_len - s_start) / seg.length, 0.0f, 1.0f);
            
            if (local_s >= local_e - 1e-6f) continue;
            
            if (seg.verb == PathVerb::LineTo) {
                const math::Vector2 p_start = seg.p[0] * (1.0f - local_s) + seg.p[1] * local_s;
                const math::Vector2 p_end = seg.p[0] * (1.0f - local_e) + seg.p[1] * local_e;
                if (first_move) {
                    out.commands.push_back({PathVerb::MoveTo, p_start});
                    first_move = false;
                }
                out.commands.push_back({PathVerb::LineTo, p_end});
            } else if (seg.verb == PathVerb::CubicTo) {
                math::Vector2 sub[4];
                extract_sub_cubic(seg.p[0], seg.p[1], seg.p[2], seg.p[3], local_s, local_e, sub);
                if (first_move) {
                    out.commands.push_back({PathVerb::MoveTo, sub[0]});
                    first_move = false;
                }
                out.commands.push_back({PathVerb::CubicTo, sub[1], sub[2], sub[3]});
            }
        }
    };
    
    PathGeometry result;
    if (t_start <= t_end) {
        collect_range(t_start, t_end, result);
    } else {
        collect_range(t_start, 1.0f, result);
        collect_range(0.0f, t_end, result);
    }
    
    return result;
}

} // namespace tachyon::renderer2d
