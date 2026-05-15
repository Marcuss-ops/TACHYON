#include "tachyon/media/audio_ops.h"
#include "tachyon/core/audio/audio_analyzer.h"

namespace tachyon::ops {

core::MediaResult<std::string> AudioOps::transcribe(const std::filesystem::path& path) {
    // Delega al cuore di Tachyon
    return core::audio::AudioAnalyzer::transcribe(path);
}

} // namespace tachyon::ops
