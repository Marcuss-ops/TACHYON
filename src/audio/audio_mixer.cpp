#include "tachyon/audio/audio_mixer.h"
#include "tachyon/audio/dsp_nodes.h"

#include <algorithm>
#include <cmath>

namespace tachyon::audio {

AudioMixer::AudioMixer() {
    // Add a default limiter to the master bus to prevent clipping
    m_graph.master().add_node(std::make_shared<LimiterNode>(1.0f));
}
AudioMixer::~AudioMixer() = default;

void AudioMixer::add_track(std::shared_ptr<AudioDecoder> decoder, const AudioTrackMixParams& params, const std::string& bus_id) {
    if (decoder) {
        m_tracks.push_back({std::move(decoder), params, bus_id});
    }
}

void AudioMixer::clear_tracks() {
    m_tracks.clear();
}

void AudioMixer::mix(double startTimeSeconds, double durationSeconds, std::vector<float>& out_stereo_pcm) {
    const std::size_t target_samples = static_cast<std::size_t>(std::ceil(durationSeconds * kTargetSampleRate));
    const std::size_t target_floats = target_samples * kTargetChannels;
    
    out_stereo_pcm.assign(target_floats, 0.0f);

    for (const auto& track : m_tracks) {
        if (!track.params.enabled || track.params.volume <= 0.0f) {
            continue;
        }

        const double track_start = track.params.start_offset_seconds;
        const double track_end = track_start + track.decoder->duration();
        const double request_end = startTimeSeconds + durationSeconds;
        
        if (track_end <= startTimeSeconds || track_start >= request_end) {
            continue;
        }

        const double source_start_time = std::max(0.0, startTimeSeconds - track_start);
        const double decode_duration = std::min(track.decoder->duration() - source_start_time, durationSeconds);
        
        if (decode_duration <= 0.0) {
            continue;
        }

        std::vector<float> decoded = track.decoder->decode_range(source_start_time, decode_duration);
        if (decoded.empty()) {
            continue;
        }

        // Apply per-track bus processing (EQ, Compression, etc.)
        // This ensures that track-level effects are applied before summation.
        if (track.bus_id != "master") {
            m_graph.process_track(track.bus_id, decoded.data(), static_cast<int>(decoded.size() / kTargetChannels));
        }

        const double out_offset_sec = std::max(0.0, track_start - startTimeSeconds);
        const std::size_t out_offset_samples = static_cast<std::size_t>(std::lround(out_offset_sec * kTargetSampleRate));
        const std::size_t out_offset_floats = out_offset_samples * kTargetChannels;

        const float volume = track.params.volume;
        const std::size_t mix_count = std::min(decoded.size(), out_stereo_pcm.size() - out_offset_floats);

        for (std::size_t i = 0; i < mix_count; ++i) {
            out_stereo_pcm[out_offset_floats + i] += decoded[i] * volume;
        }
    }

    // Apply AudioGraph master bus processing
    m_graph.master().process(out_stereo_pcm.data(), static_cast<int>(target_samples));

    // Final Clamping to [-1.0, 1.0]
    for (float& sample : out_stereo_pcm) {
        if (sample > 1.0f) sample = 1.0f;
        else if (sample < -1.0f) sample = -1.0f;
    }
}

} // namespace tachyon::audio
