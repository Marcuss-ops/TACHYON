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
    (void)plan;  // ProRes is not implemented, ignore input
    impl_->last_error = "ProRes output is not implemented yet";
    return false;
}

bool ProResSink::write_frame(const OutputFramePacket& /*packet*/) {
    impl_->last_error = "ProRes output is not implemented yet";
    return false;
}

bool ProResSink::finish() {
    impl_->last_error = "ProRes output is not implemented yet";
    return false;
}

bool ProResSink::finalize_post_processing() {
    impl_->last_error = "ProRes output is not implemented yet";
    return false;
}

const std::string& ProResSink::last_error() const {
    return impl_->last_error;
}

std::unique_ptr<FrameOutputSink> create_prores_sink(const ProResSinkConfig& config) {
    return std::make_unique<ProResSink>(config);
}

} // namespace output
