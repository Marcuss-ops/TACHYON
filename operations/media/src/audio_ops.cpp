#include "tachyon/ops/audio_ops.h"
#include "tachyon/backends/whisper/whisper_audio_analyzer.h"

namespace tachyon::ops {

core::MediaResult<std::string> AudioOps::transcribe(const std::filesystem::path& path) {
    // Operations delegate to the concrete Backend analyzer
    backends::whisper::WhisperAudioAnalyzer analyzer;
    return analyzer.transcribe(path);
}

} // namespace tachyon::ops
