#include "tachyon/audio/audio_processor.h"
#include "tachyon/core/spec/schema/audio/audio_spec.h"
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>

namespace {

int g_failures = 0;
constexpr float kEps = 1e-3f;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

// Simple mock decoder that returns a unique value per second to verify seek/trim
class TrimMockAudioDecoder : public tachyon::audio::AudioDecoder {
public:
    TrimMockAudioDecoder(float duration_sec, int sample_rate)
        : m_duration(duration_sec), m_sample_rate(sample_rate) {}

    double duration() const noexcept override { return static_cast<double>(m_duration); }
    int sample_rate() const noexcept override { return m_sample_rate; }

    std::vector<float> decode_range(double start_sec, double dur_sec) override {
        const std::size_t samples = static_cast<std::size_t>(std::max(0.0, dur_sec) * m_sample_rate) * 2;
        std::vector<float> out(samples, 0.0f);
        
        // Each sample value is (start_sec + local_time)
        // This allows us to verify if the correct range was read
        for (std::size_t i = 0; i < samples / 2; ++i) {
            float local_time = static_cast<float>(i) / m_sample_rate;
            float val = static_cast<float>(start_sec) + local_time;
            out[i * 2] = val;
            out[i * 2 + 1] = val;
        }
        return out;
    }

private:
    float m_duration;
    int m_sample_rate;
};

} // namespace

bool run_audio_trim_tests() {
    using namespace tachyon;
    using namespace tachyon::audio;
    using namespace tachyon::spec;

    std::cout << "Running Audio Trim Tests...\n";

    // --- Test 1: Basic Trimming ---
    // Source: 10s. Trim: 2s to 5s.
    // Result duration should be 3s.
    {
        AudioProcessor proc;
        auto decoder = std::make_shared<TrimMockAudioDecoder>(10.0f, 44100);
        AudioTrackSpec spec;
        spec.in_point_seconds = 2.0;
        spec.out_point_seconds = 5.0;
        spec.volume = 1.0f;
        spec.pan = -1.0f; // Left only, gain = 1.0
        proc.add_track(decoder, spec);

        std::vector<float> out;
        // Process at composition time 0.0 (which should map to source time 2.0)
        proc.process(0.0, 1.0, 44100, out);
        
        check_true(!out.empty(), "Trimmed track produces output");
        check_true(out.size() == 88200, "Correct sample count for 1s");
        
        // Check first sample value. It should be ~2.0
        if (std::abs(out[0] - 2.0f) >= kEps) {
            std::cout << "DEBUG Test 1: out[0] = " << out[0] << ", expected = 2.0\n";
        }
        check_true(std::abs(out[0] - 2.0f) < kEps, "First sample value matches in_point");
        
        // Process at composition time 2.0 (which should map to source time 4.0)
        out.clear();
        proc.process(2.0, 1.0, 44100, out);
        if (std::abs(out[0] - 4.0f) >= kEps) {
            std::cout << "DEBUG Test 1: out[0] at 2s = " << out[0] << ", expected = 4.0\n";
        }
        check_true(std::abs(out[0] - 4.0f) < kEps, "Sample at comp time 2.0 matches source time 4.0");

        // Process at composition time 3.5 (past out_point). Should be silent.
        out.clear();
        proc.process(3.5, 1.0, 44100, out);
        float max_val = 0.0f;
        for (float v : out) max_val = std::max(max_val, std::abs(v));
        check_true(max_val < kEps, "Output past out_point is silent");
    }

    // --- Test 2: Trimming with Speed ---
    // Source: 10s. Trim: 2s to 6s (source duration 4s). Speed: 2x.
    // Result duration should be 2s.
    {
        AudioProcessor proc;
        auto decoder = std::make_shared<TrimMockAudioDecoder>(10.0f, 44100);
        AudioTrackSpec spec;
        spec.in_point_seconds = 2.0;
        spec.out_point_seconds = 6.0;
        spec.playback_speed = 2.0f;
        spec.volume = 1.0f;
        spec.pan = -1.0f; // Left only, gain = 1.0
        proc.add_track(decoder, spec);

        std::vector<float> out;
        // Process at composition time 0.0 (source time 2.0)
        proc.process(0.0, 1.0, 44100, out);
        if (std::abs(out[0] - 2.0f) >= kEps) {
            std::cout << "DEBUG Test 2: out[0] = " << out[0] << ", expected = 2.0\n";
        }
        check_true(std::abs(out[0] - 2.0f) < kEps, "Speed 2x: First sample matches in_point");
        
        // At comp time 0.5s, source time should be 2.0 + 0.5 * 2 = 3.0
        out.clear();
        proc.process(0.5, 0.1, 44100, out);
        if (std::abs(out[0] - 3.0f) >= kEps) {
            std::cout << "DEBUG Test 2: out[0] at 0.5s = " << out[0] << ", expected = 3.0\n";
        }
        check_true(std::abs(out[0] - 3.0f) < kEps, "Speed 2x: Sample at comp 0.5s matches source 3.0s");

        // At comp time 2.1s (past 2s duration). Should be silent.
        out.clear();
        proc.process(2.1, 0.1, 44100, out);
        float max_val = 0.0f;
        for (float v : out) max_val = std::max(max_val, std::abs(v));
        check_true(max_val < kEps, "Speed 2x: Output past trimmed duration is silent");
    }

    return g_failures == 0;
}
