#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include <memory>
#include <string>
#include <vector>

namespace tachyon::output {

// A named AOV (Arbitrary Output Variable) surface accompanying a beauty frame.
// Standard AOV names:
//   "beauty"         – full RGBA composite (always present in RasterizedFrame2D.surface)
//   "depth"          – linear depth from camera, R channel (float32)
//   "normal"         – world-space normals, RGB channels
//   "motion_vector"  – screen-space velocity, RG channels (pixels/frame, centred at 0.5)
//   "alpha_matte"    – extracted alpha from keyed layer, A channel
//   "shadow"         – shadow pass, R channel
//   "object_id"      – per-object integer ID encoded as float, R channel
struct FrameAOV {
    std::string name;
    std::shared_ptr<renderer2d::SurfaceRGBA> surface;
};

} // namespace tachyon::output
