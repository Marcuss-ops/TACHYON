#pragma once

#include "tachyon/audio/audio_decoder.h"
#include "tachyon/audio/audio_graph.h"

#include <vector>
#include <memory>

namespace tachyon::audio {

struct AudioTrackMixParams {
    double start_offset_seconds{0.0};
    float volume{1.0f};
    bool enabled{true};
};

class AudioMixer {
public:
    AudioMixer();
    ~AudioMixer();

    void add_track(std::shared_ptr<AudioDecoder> decoder, const AudioTrackMixParams& params, const std::string& bus_id = "master");
    
    // Mixes stereo 48kHz audio into the output buffer for the given time range.
    // out_stereo_pcm must have at least static_cast<size_t>(duration_seconds * 48000) * 2 capacity.
    void mix(double startTimeSeconds, double durationSeconds, std::vector<float>& out_stereo_pcm);

    void clear_tracks();
    
    AudioGraph& master_graph() { return m_graph; }

private:
    struct Track {
        std::shared_ptr<AudioDecoder> decoder;
        AudioTrackMixParams params;
        std::string bus_id;
    };

    std::vector<Track> m_tracks;
    AudioGraph m_graph;
    
    static constexpr int kTargetSampleRate = 48000;
    static constexpr int kTargetChannels = 2;
};

} // namespace tachyon::audio
