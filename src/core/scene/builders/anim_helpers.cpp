#include "tachyon/scene/builder.h"
#include "tachyon/core/animation/easing.h"
#include <functional>

namespace tachyon::scene::anim {

AnimatedScalarSpec scalar(double v) {
    AnimatedScalarSpec s;
    s.keyframes.push_back({0.0, v});
    return s;
}
AnimatedScalarSpec lerp(double from, double to, double duration,
                        animation::EasingPreset ease) {
    AnimatedScalarSpec s;
    s.keyframes.push_back({0.0, from});
    s.keyframes.push_back({duration * 1000.0, to});
    // Apply easing to the first keyframe (affects interpolation to the next keyframe)
    if (!s.keyframes.empty()) {
        s.keyframes[0].easing = ease;
    }
    return s;
}

AnimatedScalarSpec keyframes(std::initializer_list<std::pair<double, double>> time_value_pairs) {
    AnimatedScalarSpec s;
    for (auto p : time_value_pairs) {
        s.keyframes.push_back({p.first * 1000.0, p.second});
    }
    return s;
}

AnimatedScalarSpec from_fn(double duration, std::function<double(double t)> fn, int samples) {
    AnimatedScalarSpec s;
    for (int i = 0; i <= samples; ++i) {
        double t = (duration * i) / samples;
        s.keyframes.push_back({t * 1000.0, fn(t)});
    }
    return s;
}

} // namespace tachyon::scene::anim
