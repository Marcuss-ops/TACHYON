#include "tachyon/output/frame_output_sink.h"
#include <iostream>
#include <filesystem>
#include <vector>

namespace tachyon::output {

class EXRSequenceSink : public FrameOutputSink {
public:
    EXRSequenceSink() = default;

    bool begin(const RenderPlan& plan) override {
        m_output_dir = plan.contract.output_path;
        if (!std::filesystem::exists(m_output_dir)) {
            std::filesystem::create_directories(m_output_dir);
        }
        return true;
    }

    bool write_frame(const OutputFramePacket& packet) override {
        char buf[256];
        snprintf(buf, sizeof(buf), "frame_%05lld.exr", static_cast<long long>(packet.frame_number));
        std::filesystem::path full_path = std::filesystem::path(m_output_dir) / buf;

        // --- EXR Channel Layout ---
        // 1. R, G, B, A (float32 or float16)
        // 2. Additional Layers (AOVs):
        //    - depth.Z
        //    - normal.X, normal.Y, normal.Z
        //    - motion.U, motion.V
        //    - objectId, materialId

#ifdef TACHYON_EXR
        // Real implementation using OpenEXR / TinyEXR
        // std::vector<Header> headers;
        // for (const auto& aov : packet.aovs) { ... }
#else
        std::cout << "[EXR] Exporting " << full_path.string() << std::endl;
        std::cout << "      - RGBA (32-bit float)" << std::endl;
        for (const auto& aov : packet.aovs) {
            std::cout << "      - AOV Layer: " << aov.name << " (" << aov.surface->width() << "x" << aov.surface->height() << ")" << std::endl;
        }
#endif
        return true;
    }

    bool finish() override { return true; }
    const std::string& last_error() const override { return m_last_error; }

private:
    std::string m_output_dir;
    std::string m_last_error;
};

std::unique_ptr<FrameOutputSink> create_exr_sequence_sink() {
    return std::make_unique<EXRSequenceSink>();
}

} // namespace tachyon::output
