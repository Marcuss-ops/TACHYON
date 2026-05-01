#include "tachyon/output/prores_sink.h"
#include <memory>
#include <string>
#include <sstream>

namespace tachyon::output {

struct ProResSink::Impl {
    ProResSinkConfig config;
    std::string output_path;
    std::string last_error;
    bool initialized{false};
    // FFmpeg process handle, etc.
};

ProResSink::ProResSink(const ProResSinkConfig& config) 
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
}

ProResSink::~ProResSink() = default;

bool ProResSink::begin(const RenderPlan& plan) {
    if (plan.output.destination.path.empty()) {
        impl_->last_error = "Output path not specified";
        return false;
    }
    impl_->output_path = plan.output.destination.path;
    // TODO: Initialize FFmpeg for ProRes encoding
    impl_->initialized = true;
    return true;
}

bool ProResSink::write_frame(const OutputFramePacket& /*packet*/) {
    if (!impl_->initialized) {
        impl_->last_error = "Sink not initialized";
        return false;
    }
    // TODO: Write frame to ProRes encoder
    return true;
}

bool ProResSink::finish() {
    if (!impl_->initialized) {
        return false;
    }
    // TODO: Finalize encoding
    return true;
}

bool ProResSink::finalize_post_processing() {
    return true;
}

const std::string& ProResSink::last_error() const {
    return impl_->last_error;
}

std::unique_ptr<FrameOutputSink> create_prores_sink(const ProResSinkConfig& config) {
    return std::make_unique<ProResSink>(config);
}

} // namespace output
