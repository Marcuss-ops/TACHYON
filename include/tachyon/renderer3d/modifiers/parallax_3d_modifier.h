#pragma once

#include "tachyon/renderer3d/modifiers/i3d_modifier.h"
#include "tachyon/core/spec/schema/3d/three_d_spec.h"

namespace tachyon {
namespace renderer3d {

class Parallax3DModifier : public I3DModifier {
public:
    explicit Parallax3DModifier(const ThreeDModifierSpec& spec);

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
