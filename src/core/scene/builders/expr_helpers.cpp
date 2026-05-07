#include "tachyon/scene/builder.h"
#include <cmath>

namespace tachyon::scene::expr {

AnimatedScalarSpec wiggle(double frequency, double amplitude, int seed) {
    AnimatedScalarSpec spec;
    spec.wiggle.enabled = true;
    spec.wiggle.frequency = frequency;
    spec.wiggle.amplitude = amplitude;
    spec.wiggle.seed = seed;
    return spec;
}

AnimatedScalarSpec sin_wave(double frequency, double amplitude, double offset) {
    AnimatedScalarSpec spec;
    const double duration = 10.0;
    const int samples = static_cast<int>(duration * 30.0);
    for (int i = 0; i < samples; ++i) {
        double t = i / 30.0;
        double val = std::sin(t * frequency * 2.0 * 3.14159265358979323846 + offset) * amplitude;
        spec.keyframes.push_back({static_cast<double>(i), val});
    }
    return spec;
}

AnimatedScalarSpec pulse(double frequency, double amplitude) {
    AnimatedScalarSpec spec;
    const double duration = 10.0;
    for (int i = 0; i < static_cast<int>(duration * 30.0); ++i) {
        double t = i / 30.0;
        double val = (std::fmod(t * frequency, 1.0) < 0.5) ? amplitude : 0.0;
        spec.keyframes.push_back({static_cast<double>(i), val});
    }
    return spec;
}

} // namespace tachyon::scene::expr
