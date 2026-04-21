#include "tachyon/runtime/core/compiled_scene.h"

namespace tachyon {

bool CompiledScene::is_valid() const noexcept {
    return header.magic == 0x54414348U;
}

} // namespace tachyon
