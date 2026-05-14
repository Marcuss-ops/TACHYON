#pragma once

namespace tachyon::audio {

/**
 * @brief Frequency bands for audio-driven animation.
 */
struct AudioBands {
    float bass{0.0f};
    float mid{0.0f};
    float high{0.0f};
    float presence{0.0f};
    float rms{0.0f};
    float beat{0.0f};
};

} // namespace tachyon::audio
