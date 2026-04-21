#include "tachyon/runtime/core/compiled_scene.h"

namespace tachyon {

bool CompiledScene::is_valid() const noexcept {
    if (header.magic != 0x54414348U) return false;
    if (header.version != 1) return false;
    
    // Validate layout integrity
    if (header.layout_checksum != calculate_layout_checksum()) return false;
    
    return true;
}

} // namespace tachyon

