#pragma once

#include "tachyon/output/frame_output_sink.h"
#include <memory>

namespace tachyon::output {

std::unique_ptr<FrameOutputSink> create_shared_memory_sink();

} // namespace tachyon::output
