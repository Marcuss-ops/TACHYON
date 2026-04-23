#include "tachyon/audio/audio_mixer.h"
#include "tachyon/audio/dsp_nodes.h"
#include "tachyon/runtime/execution/presentation_clock.h"

#include <algorithm>
#include <cmath>

namespace tachyon {
namespace audio {

AudioMixer::AudioMixer() {
  // Add a default limiter to the master bus to prevent clipping
  m_graph.master().add_node(std::make_shared<LimiterNode>(1.0f));
}
AudioMixer::~AudioMixer() = default;

void AudioMixer::add_track(std::shared_ptr<AudioDecoder> decoder,
                           const AudioTrackMixParams &params,
                           const std::string &bus_id) {
  if (decoder) {
    m_tracks.push_back({std::move(decoder), params, bus_id});
  }
}

void AudioMixer::clear_tracks() { m_tracks.clear(); }

void AudioMixer::mix(double startTimeSeconds, double durationSeconds,
                     std::vector<float> &out_stereo_pcm) {
  const std::size_t target_samples =
      static_cast<std::size_t>(std::ceil(durationSeconds * kTargetSampleRate));
  const std::size_t target_floats = target_samples * kTargetChannels;

  out_stereo_pcm.assign(target_floats, 0.0f);

  for (const auto &track : m_tracks) {
    if (!track.params.enabled || track.params.volume <= 0.0f) {
      continue;
    }

    const double track_start = track.params.start_offset_seconds;
    const double track_end = track_start + track.decoder->duration();
    const double request_end = startTimeSeconds + durationSeconds;

    if (track_end <= startTimeSeconds || track_start >= request_end) {
      continue;
    }

    const float playback_speed = std::max(0.01f, track.params.playback_speed);
    const double source_start_time =
        std::max(0.0, startTimeSeconds - track_start);
    const double source_duration = durationSeconds * playback_speed;

    if (source_start_time + source_duration > track.decoder->duration()) {
      // Adjust duration if it exceeds decoder bounds
    }

    std::vector<float> decoded = track.decoder->decode_range(
        source_start_time, source_duration + 0.1); // Extra for sinc kernel
    if (decoded.empty()) {
      continue;
    }

    const double out_offset_sec = std::max(0.0, track_start - startTimeSeconds);
    const std::size_t out_offset_samples = static_cast<std::size_t>(
        std::lround(out_offset_sec * kTargetSampleRate));
    const std::size_t out_offset_floats = out_offset_samples * kTargetChannels;

    const float volume = track.params.volume;
    const float pan = std::clamp(track.params.pan, -1.0f, 1.0f);

    // Constant power panning
    const float angle = (pan + 1.0f) * (3.14159265f / 4.0f);
    const float left_gain = std::cos(angle) * volume;
    const float right_gain = std::sin(angle) * volume;

    // Windowed Sinc Resampling
    const int kernel_radius = 4;

    for (std::size_t i = 0; i < target_samples - out_offset_samples; ++i) {
      float src_pos = static_cast<float>(i) * playback_speed;
      int src_idx = static_cast<int>(std::floor(src_pos));
      float frac = src_pos - static_cast<float>(src_idx);

      float sum_l = 0.0f;
      float sum_r = 0.0f;
      float w_total = 0.0f;

      for (int k = -kernel_radius + 1; k <= kernel_radius; ++k) {
        int s_idx = src_idx + k;
        if (s_idx < 0 || s_idx >= static_cast<int>(decoded.size() / 2))
          continue;

        float x = static_cast<float>(k) - frac;
        // Sinc with Lanczos window
        float sinc = (std::abs(x) < 1e-6f)
                         ? 1.0f
                         : std::sin(3.14159265f * x) / (3.14159265f * x);
        float lanczos = (std::abs(x) < static_cast<float>(kernel_radius))
                            ? std::sin(3.14159265f * x / kernel_radius) /
                                  (3.14159265f * x / kernel_radius)
                            : 0.0f;
        float weight = sinc * lanczos;

        sum_l += decoded[s_idx * 2] * weight;
        sum_r += decoded[s_idx * 2 + 1] * weight;
        w_total += weight;
      }

      if (std::abs(w_total) < 1e-6f) {
        continue;
      }

      const std::size_t out_sample = out_offset_floats + i * 2;
      if (out_sample + 1 >= out_stereo_pcm.size()) {
        break;
      }

      const float norm = 1.0f / w_total;
      out_stereo_pcm[out_sample] += sum_l * left_gain * norm;
      out_stereo_pcm[out_sample + 1] += sum_r * right_gain * norm;
    }
  }
}

} // namespace audio
} // namespace tachyon
