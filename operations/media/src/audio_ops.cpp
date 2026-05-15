#include "tachyon/ops/audio_ops.h"
#include "tachyon/core/media/audio_analyzer.h"

namespace tachyon::ops {

core::MediaResult<std::string> AudioOps::transcribe(const std::filesystem::path& path) {
    // Operations delegate to the Core Media analyzer
    return core::media::AudioAnalyzer::transcribe(path);
}

} // namespace tachyon::ops
