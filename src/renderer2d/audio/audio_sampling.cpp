#include "tachyon/renderer2d/audio/audio_sampling.h"

namespace tachyon::renderer2d::audio {

double sample_audio_band(const ::tachyon::audio::AudioBands& bands, AudioBandType band) {
    switch (band) {
        case AudioBandType::Bass: return bands.bass;
        case AudioBandType::Mid: return bands.mid;
        case AudioBandType::High: return bands.high;
        case AudioBandType::Presence: return bands.presence;
        case AudioBandType::Rms: return bands.rms;
        default: break;
    }
    return bands.rms;
}

} // namespace tachyon::renderer2d::audio