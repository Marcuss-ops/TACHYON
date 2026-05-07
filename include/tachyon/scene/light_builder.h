#pragma once

#include "tachyon/core/api.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/types/colors.h"
#include "tachyon/core/math/algebra/vector3.h"

namespace tachyon::scene {

class LayerBuilder;

/**
 * @brief Builder for light-specific properties on a LayerSpec.
 */
class TACHYON_API LightBuilder {
    LayerBuilder& parent_;
public:
    explicit LightBuilder(LayerBuilder& parent) : parent_(parent) {}

    LightBuilder& type(std::string t);
    LightBuilder& color(const ColorSpec& c);
    LightBuilder& intensity(double i);
    LightBuilder& casts_shadows(bool v);
    LightBuilder& shadow_radius(double r);

    LayerBuilder& done();
};

} // namespace tachyon::scene
