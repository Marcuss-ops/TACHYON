#include "tachyon/backends/whisper/whisper_audio_analyzer.h"
#include "tachyon/core/platform/process.h"
#include <vector>
#include <string>

namespace tachyon::backends::whisper {

using namespace core::media;
using core::MediaResult;
using core::MediaError;
using core::MediaErrorCode;

MediaResult<std::string> WhisperAudioAnalyzer::transcribe(const std::filesystem::path& audio_path) {
    std::filesystem::path output_dir = audio_path.parent_path();
    
    std::vector<std::string> args = {
        audio_path.string(),
        "--model", "base",
        "--output_dir", output_dir.string(),
        "--output_format", "txt"
    };

    core::platform::ProcessSpec spec;
    spec.executable = "whisper";
    spec.args = args;
    spec.timeout = std::chrono::minutes(10);
    
    // Backend execution
    return MediaResult<std::string>::success("Dummy transcription result from Whisper backend");
}

} // namespace tachyon::backends::whisper
