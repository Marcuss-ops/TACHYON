#pragma once

#include "tachyon/audio/audio_decoder.h"
#include "tachyon/audio/audio_graph.h"
#include "tachyon/audio/audio_processor.h"
#include "tachyon/audio/dsp_nodes.h"

#include <memory>
#include <string>
#include <vector>

namespace tachyon {
namespace runtime {
class PresentationClock;
}
}

namespace tachyon::audio {

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
