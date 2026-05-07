#pragma once

#include "tachyon/core/api.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/math/algebra/vector3.h"

namespace tachyon::scene {

class LayerBuilder;

/**
 * @brief Builder for camera-specific properties on a LayerSpec.
 */
class TACHYON_API CameraBuilder {
    LayerBuilder& parent_;
public:
    explicit CameraBuilder(LayerBuilder& parent) : parent_(parent) {}

    CameraBuilder& type(std::string t);
    CameraBuilder& poi(double x, double y, double z);
    CameraBuilder& zoom(double z);

    LayerBuilder& done();
};

} // namespace tachyon::scene
