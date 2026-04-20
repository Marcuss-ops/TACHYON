#include "tachyon/audio/audio_mixer.h"

#include <algorithm>
#include <cmath>

namespace tachyon::audio {

AudioMixer::AudioMixer() = default;
AudioMixer::~AudioMixer() = default;

void AudioMixer::add_track(std::shared_ptr<AudioDecoder> decoder, const AudioTrackMixParams& params) {
    if (decoder) {
        m_tracks.push_back({std::move(decoder), params});
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

        // Calculate the slice of the track that overlaps with our requested range
        // Track starts at track.params.start_offset_seconds relative to composition 0
        const double track_start = track.params.start_offset_seconds;
        const double track_end = track_start + track.decoder->duration();
        
        const double request_end = startTimeSeconds + durationSeconds;
        
        // No overlap
        if (track_end <= startTimeSeconds || track_start >= request_end) {
            continue;
        }

        // Determine the range to decode from the track
        // The track time T corresponds to composition time T + track_start
        // So source_time = composition_time - track_start
        const double source_start_time = std::max(0.0, startTimeSeconds - track_start);
        const double decode_duration = std::min(track.decoder->duration() - source_start_time, durationSeconds);
        
        if (decode_duration <= 0.0) {
            continue;
        }

        std::vector<float> decoded = track.decoder->decode_range(source_start_time, decode_duration);
        if (decoded.empty()) {
            continue;
        }

        // Offset in the output buffer where this track starts contributing
        // If startTimeSeconds < track_start, we start at (track_start - startTimeSeconds)
        const double out_offset_sec = std::max(0.0, track_start - startTimeSeconds);
        const std::size_t out_offset_samples = static_cast<std::size_t>(std::lround(out_offset_sec * kTargetSampleRate));
        const std::size_t out_offset_floats = out_offset_samples * kTargetChannels;

        const float volume = track.params.volume;
        const std::size_t mix_count = std::min(decoded.size(), out_stereo_pcm.size() - out_offset_floats);

        for (std::size_t i = 0; i < mix_count; ++i) {
            out_stereo_pcm[out_offset_floats + i] += decoded[i] * volume;
        }
    }

    // Optional: Final Clamping to [-1.0, 1.0]
    for (float& sample : out_stereo_pcm) {
        if (sample > 1.0f) sample = 1.0f;
        else if (sample < -1.0f) sample = -1.0f;
    }
}

} // namespace tachyon::audio
