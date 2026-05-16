#pragma once

#include "tachyon/core/media/media_interfaces.h"

namespace tachyon::backends::simd {


#if defined(TACHYON_ENABLE_HIGHWAY)
class HighwayTransitionRenderer : public core::media::ITransitionRenderer {
public:
    core::MediaResult<renderer2d::SurfaceRGBA> render(
        const renderer2d::SurfaceRGBA& from,
        const renderer2d::SurfaceRGBA* to,
        float progress
    ) override;
};
#endif

} // namespace tachyon::backends::simd
