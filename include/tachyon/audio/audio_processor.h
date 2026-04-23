#pragma once

#include "tachyon/audio/audio_decoder.h"
#include "tachyon/audio/audio_graph.h"
#include "tachyon/core/spec/schema/contracts/shared_contracts.h"

#include <memory>
#include <vector>
#include <string>

namespace tachyon::audio {

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
    AudioProcessor();
    ~AudioProcessor();

    // Setup
    void add_track(std::shared_ptr<AudioDecoder> decoder, const spec::AudioTrackSpec& spec, const std::string& bus_id = "master");
    void clear_tracks();

    // Processing
    // Mixes audio for the given time range into out_stereo_pcm.
    // out_stereo_pcm will be resized to (durationSeconds * sampleRate * 2).
    void process(double startTimeSeconds, double durationSeconds, int sampleRate, std::vector<float>& out_stereo_pcm);

    // DSP Access
    AudioGraph& graph() { return m_graph; }

private:
    // Internal resampling and mixing logic
    void mix_track(const TrackInstance& track, double startTimeSeconds, double durationSeconds, int sampleRate, std::vector<float>& mix_buffer);
    
    // WSOLA Time-stretch implementation
    void apply_time_stretch(const std::vector<float>& input, float speed, std::vector<float>& output);

    std::vector<TrackInstance> m_tracks;
    AudioGraph m_graph;

    static constexpr int kInternalSampleRate = 48000;
};

} // namespace tachyon::audio
