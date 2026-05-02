#pragma once

#include "tachyon/renderer3d/modifiers/i3d_modifier.h"
#include "tachyon/core/spec/schema/3d/three_d_spec.h"
#include <memory>
#include <functional>
#include <unordered_map>
#include <string>

namespace tachyon {
namespace renderer3d {

class ThreeDModifierRegistry {
public:
    using Factory = std::function<std::unique_ptr<I3DModifier>(const ThreeDModifierSpec&)>;

    static ThreeDModifierRegistry& instance();

    void registerModifier(const std::string& type, Factory factory);
    std::unique_ptr<I3DModifier> create(const ThreeDModifierSpec& spec) const;

private:
    ThreeDModifierRegistry() = default;
    std::unordered_map<std::string, Factory> factories_;
};

} // namespace renderer3d
} // namespace tachyon
