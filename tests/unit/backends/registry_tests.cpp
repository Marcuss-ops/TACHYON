#include "test_utils.h"
#include "tachyon/backends/backend_registry.h"
#include "tachyon/core/media/media_interfaces.h"
#include "tachyon/renderer2d/core/framebuffer.h"

using namespace tachyon::backends;
using namespace tachyon::core;
using namespace tachyon::core::media;

// Mock implementations for testing
class MockProbe : public IMediaProbe {
public:
    MediaResult<FullMetadata> probe_file(const std::filesystem::path& path) override { return {}; }
    MediaResult<FullMetadata> probe_full(const std::filesystem::path& path) override { return {}; }
};

class MockAudioAnalyzer : public IAudioAnalyzer {
public:
    MediaResult<std::vector<float>> analyze_waveform(const std::filesystem::path& path) override { return {}; }
    MediaResult<std::string> transcribe(const std::filesystem::path& path) override { return {}; }
};

class MockEncoder : public IVideoEncoder {
public:
    MediaResult<void> open(const std::filesystem::path& output_path, int width, int height, double fps) override { return MediaResult<void>::success(); }
    MediaResult<void> write_frame(const tachyon::renderer2d::SurfaceRGBA& surface) override { return MediaResult<void>::success(); }
    void close() override {}
};

namespace tachyon::backends {

bool run_backend_registry_tests() {
    auto& reg = BackendRegistry::instance();
    
    // Test: Deterministic Defaults
    {
        reg.register_probe("mock_probe", []() { return std::make_unique<MockProbe>(); });
        reg.set_default_probe("mock_probe");
        
        auto probe = reg.create_probe("default");
        if (probe == nullptr) return false;
    }
    
    // Test: Fallback when not registered
    {
        reg.set_default_probe("non_existent");
        auto probe = reg.create_probe("default");
        if (probe != nullptr) return false;
        
        auto probe2 = reg.create_probe("something_else");
        if (probe2 != nullptr) return false;
    }
    
    // Test: List Capabilities
    {
        reg.register_probe("p1", []() { return std::make_unique<MockProbe>(); });
        reg.register_probe("p2", []() { return std::make_unique<MockProbe>(); });
        
        auto probes = reg.list_probes();
        bool found_p1 = false;
        bool found_p2 = false;
        for (const auto& p : probes) {
            if (p == "p1") found_p1 = true;
            if (p == "p2") found_p2 = true;
        }
        if (!found_p1 || !found_p2) return false;
    }

    return true;
}

} // namespace tachyon::backends
