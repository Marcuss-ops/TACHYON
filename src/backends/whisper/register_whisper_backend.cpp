#include "tachyon/backends/backend_registry.h"
#include "tachyon/backends/whisper/whisper_audio_analyzer.h"

namespace tachyon::backends::whisper {

void register_backend() {
    auto& reg = BackendRegistry::instance();

    reg.register_audio_analyzer("whisper", []() {
        return std::make_unique<WhisperAudioAnalyzer>();
    });
}

} // namespace tachyon::backends::whisper
