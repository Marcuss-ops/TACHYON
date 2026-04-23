#pragma once

#include "tachyon/audio/audio_decoder.h"
#include "tachyon/audio/audio_graph.h"
#include "tachyon/audio/audio_processor.h"

#include <memory>
#include <string>
#include <vector>

namespace tachyon {
namespace runtime {
class PresentationClock;
}
}

namespace tachyon::audio {

struct AudioTrackMixParams {
  double start_offset_seconds{0.0};
  float volume{1.0f};
  float pan{0.0f};            // -1.0 (L) to 1.0 (R)
  float playback_speed{1.0f}; // 1.0 = normal, 0.5 = slow, 2.0 = fast
  bool enabled{true};
};

class AudioMixer {
public:
  AudioMixer();
  ~AudioMixer();

  void add_track(std::shared_ptr<AudioDecoder> decoder,
                 const AudioTrackMixParams &params,
                 const std::string &bus_id = "master");

  void mix(double startTimeSeconds, double durationSeconds,
           std::vector<float> &out_stereo_pcm);

  void clear_tracks();

  AudioGraph &master_graph() { return m_processor.graph(); }

  void set_presentation_clock(runtime::PresentationClock *clock) {
    m_presentation_clock = clock;
  }

private:
  AudioProcessor m_processor;
  runtime::PresentationClock *m_presentation_clock{nullptr};

  static constexpr int kTargetSampleRate = 48000;
};

} // namespace tachyon::audio
