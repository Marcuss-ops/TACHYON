#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/media/audio_types.h"

namespace tachyon::renderer2d::audio {

double sample_audio_band(const ::tachyon::audio::AudioBands& bands, AudioBandType band);

} // namespace tachyon::renderer2d::audio