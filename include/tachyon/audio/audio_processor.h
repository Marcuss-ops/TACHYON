#pragma once

#include "tachyon/audio/audio_decoder.h"
#include "tachyon/audio/audio_graph.h"
#include "tachyon/core/spec/schema/contracts/shared_contracts.h"

#include <memory>
#include <vector>
#include <string>
#include <cstdint>

namespace tachyon::audio {

enum class AudioStretchMode { kResample, kWSOLA, kPhaseVocoder };

struct AudioProcessorConfig {
    AudioStretchMode mode{AudioStretchMode::kWSOLA};
    int frame_size{1024};
    int overlap{512};
    float quality_threshold{0.5f};
    bool enable_pitch_correct{true};
};

/**
 * @brief Configuration for a single track instance in the processor.
 */
struct TrackInstance {
    std::shared_ptr<AudioDecoder> decoder;
    spec::AudioTrackSpec spec;
    std::string bus_id{"master"};
};

/**
 * @brief Unified engine for mixing and processing audio.
 * Shared between AudioMixer (preview) and AudioExporter (render).
 */
class AudioProcessor {
public:
    explicit AudioProcessor(AudioProcessorConfig config = {});
    ~AudioProcessor();

    // Setup
    void add_track(std::shared_ptr<AudioDecoder> decoder, const spec::AudioTrackSpec& spec, const std::string& bus_id = "master");
    void clear_tracks();
    void set_config(const AudioProcessorConfig& config) { config_ = config; }

    // Processing
    // Mixes audio for the given time range into out_stereo_pcm.
    // out_stereo_pcm will be resized to (durationSeconds * sampleRate * 2).
    void process(double startTimeSeconds, double durationSeconds, int sampleRate, std::vector<float>& out_stereo_pcm);

    // Fast resampling for scrub preview (reversible, lower quality)
    void resample_for_scrub(const std::vector<float>& input, float speed, std::vector<float>& output);

    // Pitch-correct time stretch (WSOLA)
    void time_stretch(float speed, float pitch_ratio, const std::vector<float>& input, std::vector<float>& output);

    // Reverse playback (explicit case)
    void reverse_audio(std::vector<float>& audio);

    // DSP Access
    AudioGraph& graph() { return m_graph; }

private:
    // Internal resampling and mixing logic
    void mix_track(const TrackInstance& track, double startTimeSeconds, double durationSeconds, int sampleRate, std::vector<float>& mix_buffer);
    
    // WSOLA Time-stretch implementation
    void apply_wsola(const std::vector<float>& input, float speed, std::vector<float>& output);
    
    // Fallback resampling when quality is low
    void fallback_resample(const std::vector<float>& input, float ratio, std::vector<float>& output);

    std::vector<TrackInstance> m_tracks;
    AudioGraph m_graph;
    AudioProcessorConfig config_;

    static constexpr int kInternalSampleRate = 48000;
};

} // namespace tachyon::audio
