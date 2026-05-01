#include "tachyon/audio/audio_processor.h"
#include "tachyon/audio/audio_mixer.h"
#include "tachyon/core/spec/schema/audio/audio_spec.h"
#include "tachyon/runtime/core/serialization/tbf_codec.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <cstring>

namespace {

int g_failures = 0;
constexpr float kEps = 1e-3f;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

bool nearly_equal(float a, float b, float eps = kEps) {
    return std::abs(a - b) <= eps;
}

// Simple mock decoder that returns a sine wave or silence
class MockAudioDecoder : public tachyon::audio::AudioDecoder {
public:
    MockAudioDecoder(float duration_sec, int sample_rate, bool silent = false)
        : m_duration(duration_sec), m_sample_rate(sample_rate), m_silent(silent) {}

    double duration() const noexcept override { return static_cast<double>(m_duration); }
    int sample_rate() const noexcept override { return m_sample_rate; }

    std::vector<float> decode_range(double start_sec, double dur_sec) override {
        const std::size_t samples = static_cast<std::size_t>(std::max(0.0, dur_sec) * m_sample_rate) * 2;
        std::vector<float> out(samples, 0.0f);
        if (!m_silent) {
            const std::size_t start_sample = static_cast<std::size_t>(std::max(0.0, start_sec) * m_sample_rate);
            for (std::size_t i = 0; i < samples / 2; ++i) {
                float v = std::sin(2.0f * 3.14159265f * 440.0f * static_cast<float>(start_sample + i) / static_cast<float>(m_sample_rate));
                out[i * 2] = v;
                out[i * 2 + 1] = v;
            }
        }
        return out;
    }

private:
    float m_duration;
    int m_sample_rate;
    bool m_silent;
};

} // namespace

bool run_audio_pitch_correct_tests() {
    using namespace tachyon;
    using namespace tachyon::audio;
    using namespace tachyon::spec;
    using namespace tachyon::runtime;

    // --- Test 1: 1x speed (no change) ---
    {
        AudioProcessor proc;
        auto decoder = std::make_shared<MockAudioDecoder>(1.0f, 48000, false);
        AudioTrackSpec spec;
        spec.playback_speed = 1.0f;
        spec.pitch_shift = 1.0f;
        spec.pitch_correct = false;
        spec.volume = 1.0f;
        proc.add_track(decoder, spec);

        std::vector<float> out;
        proc.process(0.0, 0.1, 48000, out);
        check_true(!out.empty(), "1x speed produces non-empty output");
        check_true(out.size() == 9600, "1x speed produces correct sample count");

        // Check not silence
        float max_val = 0.0f;
        for (float v : out) max_val = std::max(max_val, std::abs(v));
        check_true(max_val > 0.1f, "1x speed output contains audio signal");
    }

    // --- Test 2: 0.5x speed (slow) ---
    {
        AudioProcessor proc;
        auto decoder = std::make_shared<MockAudioDecoder>(2.0f, 48000, false);
        AudioTrackSpec spec;
        spec.playback_speed = 0.5f;
        spec.pitch_shift = 1.0f;
        spec.pitch_correct = false;
        spec.volume = 1.0f;
        proc.add_track(decoder, spec);

        std::vector<float> out;
        proc.process(0.0, 0.2, 48000, out);
        check_true(!out.empty(), "0.5x speed produces non-empty output");
    }

    // --- Test 3: 2x speed (fast) ---
    {
        AudioProcessor proc;
        auto decoder = std::make_shared<MockAudioDecoder>(1.0f, 48000, false);
        AudioTrackSpec spec;
        spec.playback_speed = 2.0f;
        spec.pitch_shift = 1.0f;
        spec.pitch_correct = false;
        spec.volume = 1.0f;
        proc.add_track(decoder, spec);

        std::vector<float> out;
        proc.process(0.0, 0.1, 48000, out);
        check_true(!out.empty(), "2x speed produces non-empty output");
    }

    // --- Test 4: Silence input ---
    {
        AudioProcessor proc;
        auto decoder = std::make_shared<MockAudioDecoder>(1.0f, 48000, true);
        AudioTrackSpec spec;
        spec.playback_speed = 1.0f;
        spec.pitch_shift = 1.0f;
        spec.pitch_correct = false;
        spec.volume = 1.0f;
        proc.add_track(decoder, spec);

        std::vector<float> out;
        proc.process(0.0, 0.1, 48000, out);
        check_true(!out.empty(), "silence input produces non-empty buffer");
        float max_val = 0.0f;
        for (float v : out) max_val = std::max(max_val, std::abs(v));
        check_true(max_val < 1e-6f, "silence input produces silent output");
    }

    // --- Test 5: Reverse audio helper ---
    {
        AudioProcessor proc;
        std::vector<float> audio = {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f};
        proc.reverse_audio(audio);
        check_true(nearly_equal(audio[0], 0.5f), "reverse_audio first element");
        check_true(nearly_equal(audio[5], 0.0f), "reverse_audio last element");
    }

    // --- Test 6: pitch_correct=true with minimal input ---
    {
        AudioProcessor proc;
        auto decoder = std::make_shared<MockAudioDecoder>(0.01f, 48000, false);
        AudioTrackSpec spec;
        spec.playback_speed = 2.0f;
        spec.pitch_shift = 1.0f;
        spec.pitch_correct = true;
        spec.volume = 1.0f;
        proc.add_track(decoder, spec);

        std::vector<float> out;
        proc.process(0.0, 0.005, 48000, out);
        // Should not crash; may be empty for very short input
        check_true(out.empty() || out.size() == 480, "pitch_correct=true with minimal input is stable");
    }

    // --- Test 7: Validation - playback_speed <= 0 should fail fast ---
    {
        AudioProcessor proc;
        auto decoder = std::make_shared<MockAudioDecoder>(1.0f, 48000, false);
        AudioTrackSpec spec;
        spec.playback_speed = 0.0f; // invalid
        spec.pitch_shift = 1.0f;
        spec.pitch_correct = false;
        spec.volume = 1.0f;
        proc.add_track(decoder, spec);

        std::vector<float> out;
        proc.process(0.0, 0.1, 48000, out);
        float max_val = 0.0f;
        for (float v : out) max_val = std::max(max_val, std::abs(v));
        check_true(max_val < 1e-6f, "playback_speed=0 produces silence (fail fast)");
    }

    // --- Test 8: Validation - pitch_shift <= 0 should fail fast ---
    {
        AudioProcessor proc;
        auto decoder = std::make_shared<MockAudioDecoder>(1.0f, 48000, false);
        AudioTrackSpec spec;
        spec.playback_speed = 1.0f;
        spec.pitch_shift = 0.0f; // invalid
        spec.pitch_correct = false;
        spec.volume = 1.0f;
        proc.add_track(decoder, spec);

        std::vector<float> out;
        proc.process(0.0, 0.1, 48000, out);
        float max_val = 0.0f;
        for (float v : out) max_val = std::max(max_val, std::abs(v));
        check_true(max_val < 1e-6f, "pitch_shift=0 produces silence (fail fast)");
    }

    // --- Test 9: TBF roundtrip for AudioTrackSpec ---
    {
        CompiledScene scene;
        scene.header.magic = 0x54414348U;
        scene.header.version = TBFCodec::current_version();
        scene.header.layout_checksum = CompiledScene::calculate_layout_checksum();
        scene.scene_hash = 12345;

        CompiledComposition comp;
        comp.width = 1920;
        comp.height = 1080;
        comp.fps = 30;
        comp.duration = 5.0;

        AudioTrackSpec track;
        track.id = "track_1";
        track.source_path = "audio/test.wav";
        track.volume = 0.8f;
        track.pan = -0.3f;
        track.start_offset_seconds = 0.5;
        track.playback_speed = 1.5f;
        track.pitch_shift = 0.9f;
        track.pitch_correct = true;
        comp.audio_tracks.push_back(track);

        scene.compositions.push_back(comp);

        auto encoded = TBFCodec::encode(scene);
        check_true(!encoded.empty(), "TBF encode produces non-empty buffer");

        auto decoded = TBFCodec::decode(encoded);
        check_true(decoded.has_value(), "TBF decode succeeds");
        if (decoded) {
            const auto& dtrack = decoded->compositions[0].audio_tracks[0];
            check_true(dtrack.id == track.id, "TBF roundtrip id");
            check_true(nearly_equal(dtrack.playback_speed, track.playback_speed), "TBF roundtrip playback_speed");
            check_true(nearly_equal(dtrack.pitch_shift, track.pitch_shift), "TBF roundtrip pitch_shift");
            check_true(dtrack.pitch_correct == track.pitch_correct, "TBF roundtrip pitch_correct");
        }
    }

    // --- Test 10: TBF backward compatibility v3 -> v4 migration ---
    {
        CompiledScene scene_to_migrate;
        scene_to_migrate.header.magic = 0x54414348U;
        scene_to_migrate.header.version = 3; 
        scene_to_migrate.header.layout_checksum = CompiledScene::calculate_layout_checksum();
        scene_to_migrate.scene_hash = 999;

        CompiledComposition comp_to_migrate;
        comp_to_migrate.audio_tracks.push_back(AudioTrackSpec{});
        comp_to_migrate.audio_tracks[0].id = "legacy";
        comp_to_migrate.audio_tracks[0].playback_speed = 1.0f; 
        comp_to_migrate.audio_tracks[0].pitch_shift = 1.0f;
        comp_to_migrate.audio_tracks[0].pitch_correct = false;
        scene_to_migrate.compositions.push_back(comp_to_migrate);

        auto migrated_scene = TBFCodec::migrate(scene_to_migrate, 3);
        check_true(migrated_scene.header.version == TBFCodec::current_version(), "migration updates version to current");
        check_true(migrated_scene.compositions[0].audio_tracks[0].playback_speed == 1.0f, "migration keeps default playback_speed");
        check_true(migrated_scene.compositions[0].audio_tracks[0].pitch_shift == 1.0f, "migration keeps default pitch_shift");
        check_true(migrated_scene.compositions[0].audio_tracks[0].pitch_correct == false, "migration keeps default pitch_correct");
    }

    // --- Test 11: Clipping/bounds - very high volume should not produce NaN/Inf ---
    {
        AudioProcessor proc;
        auto decoder = std::make_shared<MockAudioDecoder>(1.0f, 48000, false);
        AudioTrackSpec spec;
        spec.playback_speed = 1.0f;
        spec.pitch_shift = 1.0f;
        spec.pitch_correct = false;
        spec.volume = 100.0f; // very high
        proc.add_track(decoder, spec);

        std::vector<float> out;
        proc.process(0.0, 0.01, 48000, out);
        bool valid = true;
        for (float v : out) {
            if (std::isnan(v) || std::isinf(v)) {
                valid = false;
                break;
            }
        }
        check_true(valid, "high volume does not produce NaN/Inf");
    }

    return g_failures == 0;
}
