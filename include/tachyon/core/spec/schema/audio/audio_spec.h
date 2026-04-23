#pragma once
#include "tachyon/core/spec/schema/contracts/shared_contracts.h"

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace tachyon {

using AudioEffectSpec = spec::AudioEffectSpec;

using AudioTrackSpec = spec::AudioTrackSpec;

struct AudioExportConfig {
    std::string codec{"aac"};
    int sample_rate{48000};
    int bitrate_kbps{192};
    int channels{2};
};

} // namespace tachyon
