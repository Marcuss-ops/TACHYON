#pragma once

#include "tachyon/core/spec/schema/common/common_spec.h"

namespace tachyon {

/**
 * @brief Utility for compile-time/constexpr resolution of layer types from raw integer IDs.
 */
class LayerKindResolver {
public:
    [[nodiscard]] static constexpr LayerType resolve(std::uint32_t type_id) noexcept {
        switch (type_id) {
            case 1: return LayerType::Solid;
            case 2: return LayerType::Shape;
            case 3: return LayerType::Image;
            case 4: return LayerType::Text;
            case 5: return LayerType::Precomp;
            case 6: return LayerType::Procedural;
            case 8: return LayerType::Video;
            case 11: return LayerType::Mask;
            case 12: return LayerType::NullLayer;
            default: return LayerType::NullLayer;
        }
    }
};

} // namespace tachyon
