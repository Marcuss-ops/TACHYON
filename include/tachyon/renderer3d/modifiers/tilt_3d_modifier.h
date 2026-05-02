#pragma once

#include "tachyon/renderer3d/modifiers/i3d_modifier.h"
#include "tachyon/core/spec/schema/3d/three_d_spec.h"

namespace tachyon {
namespace renderer3d {

class Tilt3DModifier : public I3DModifier {
public:
    explicit Tilt3DModifier(const ThreeDModifierSpec& spec);

    void apply(
        Mesh3D& mesh,
        double time,
        const renderer2d::RenderContext& ctx
    ) override;

private:
    ThreeDModifierSpec spec_;
};

} // namespace renderer3d
} // namespace tachyon
