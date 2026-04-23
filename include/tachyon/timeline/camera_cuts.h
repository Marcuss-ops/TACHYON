#pragma once

#include "tachyon/camera_cut_contract.h"

// Backward compatibility - use the unified contract
namespace tachyon::timeline {
    using ::tachyon::CameraCut;
    using ::tachyon::CameraCutTimeline;
}
