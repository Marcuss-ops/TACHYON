#include <string>
#include <string_view>
#include <unordered_map>

namespace tachyon::presets::background {

std::string normalize_background_kind(std::string_view legacy_id) {
    static const std::unordered_map<std::string_view, std::string> aliases = {
        {"aura", "tachyon.background.kind.aura"},
        {"grid", "tachyon.background.kind.grid"},
        {"grid_modern", "tachyon.background.kind.grid_modern"},
        {"stars", "tachyon.background.kind.stars"},
        {"galaxy", "tachyon.background.kind.galaxy"},
        {"noise", "tachyon.background.kind.noise"},
        {"texture", "tachyon.background.kind.texture"},
        {"soft_gradient", "tachyon.background.kind.soft_gradient"},
        {"dots", "tachyon.background.kind.dots"},
        {"silk", "tachyon.background.kind.silk"},
        {"horizon", "tachyon.background.kind.horizon"},
        {"cyber", "tachyon.background.kind.cyber"},
        {"glass", "tachyon.background.kind.glass"},
        {"nebula", "tachyon.background.kind.nebula"},
        {"metal", "tachyon.background.kind.metal"},
        {"ocean", "tachyon.background.kind.ocean"},
        {"ripple_grid", "tachyon.background.kind.ripple_grid"}
    };

    auto it = aliases.find(legacy_id);
    if (it != aliases.end()) {
        return it->second;
    }
    return std::string(legacy_id);
}

} // namespace tachyon::presets::background
