#include "tachyon/presets/sfx/sfx_registry.h"

namespace tachyon::presets {

void SfxRegistry::load_builtins() {
    register_category({"tachyon.sfx.typewriting", SfxCategory::TypeWriting, "TypeWriting", ".m4a", true});
    register_category({"tachyon.sfx.mouse", SfxCategory::Mouse, "Mouse", ".m4a", true});
    register_category({"tachyon.sfx.photo", SfxCategory::Photo, "Photo", ".m4a", true});
    register_category({"tachyon.sfx.soosh", SfxCategory::Soosh, "Soosh", ".m4a", true});
    register_category({"tachyon.sfx.money", SfxCategory::MoneySound, "MoneySound", ".m4a", true});
}

} // namespace tachyon::presets
