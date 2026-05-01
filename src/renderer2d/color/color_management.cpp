#include "tachyon/renderer2d/color/color_management.h"

namespace tachyon::renderer2d {

std::unique_ptr<IColorManager> create_color_manager() {
    return std::make_unique<InternalColorManager>();
}

} // namespace tachyon::renderer2d

