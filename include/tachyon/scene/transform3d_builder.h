#pragma once

#include "tachyon/core/api.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/math/algebra/vector3.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"

#include <string>
#include <map>

namespace tachyon::scene {

class LayerBuilder;

/**
 * @brief Builder for 3D transform properties on a LayerSpec.
 */
class TACHYON_API Transform3DBuilder {
    LayerBuilder& parent_;
public:
    explicit Transform3DBuilder(LayerBuilder& parent) : parent_(parent) {}

    Transform3DBuilder& position(double x, double y, double z);
    Transform3DBuilder& rotation(double x, double y, double z);
    Transform3DBuilder& scale(double x, double y, double z);
    Transform3DBuilder& is_3d(bool v);
    Transform3DBuilder& modifier(const std::string& id, const std::map<std::string, double>& scalars = {});
    Transform3DBuilder& animation_preset(const std::string& id, double duration = 1.0, double delay = 0.0, double intensity = 1.0);

    LayerBuilder& done();
};

} // namespace tachyon::scene
