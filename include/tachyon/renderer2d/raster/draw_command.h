#pragma once

#include "tachyon/core/math/transform2.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/color/blending.h"
#include "tachyon/renderer2d/spec/gradient_spec.h"
#include "tachyon/renderer2d/raster/path_rasterizer.h"
#include "tachyon/renderer2d/core/texture_handle.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace tachyon {
namespace renderer2d {

/**
 * @brief Runtime parameters for an effect.
 */
struct EffectParams {
    std::unordered_map<std::string, float> scalars;
    std::unordered_map<std::string, Color> colors;
    std::unordered_map<std::string, std::string> strings;
    std::unordered_map<std::string, const SurfaceRGBA*> textures;
};

enum class DrawCommandKind {
    Clear,
    SolidRect,
    MaskRect,
    TexturedQuad,
    Line,
    Shape,
    Adjustment
};

#include "tachyon/renderer2d/color/blending.h"


struct ClearCommand {
    Color color{Color::transparent()};
};

struct SolidRectCommand {
    RectI rect;
    Color color{Color::white()};
    float opacity{1.0f};
};

struct MaskRectCommand {
    RectI rect;
};

struct TexturedQuadCommand {
    TextureHandle texture;
    math::Vector2 p0{math::Vector2::zero()};
    math::Vector2 p1{math::Vector2::zero()};
    math::Vector2 p2{math::Vector2::zero()};
    math::Vector2 p3{math::Vector2::zero()};
    float w0{1.0f};
    float w1{1.0f};
    float w2{1.0f};
    float w3{1.0f};
    float opacity{1.0f};
};

struct LineCommand {
    int x0{0};
    int y0{0};
    int x1{0};
    int y1{0};
    Color color{Color::white()};
};

struct ShapeCommand {
    PathGeometry geometry;
    Color fill_color{Color::white()};
    std::optional<GradientSpec> gradient_fill;
    Color stroke_color{Color::white()};
    std::optional<GradientSpec> gradient_stroke;
    float stroke_width{0.0f};
    LineCap line_cap{LineCap::Butt};
    LineJoin line_join{LineJoin::Miter};
    float miter_limit{4.0f};
    float opacity{1.0f};
    math::Transform2 transform; // To apply to the points
};

struct AdjustmentCommand {
    std::string layer_id;
    std::vector<std::pair<std::string, EffectParams>> effects;
};

struct DrawCommand2D {
    DrawCommandKind kind{DrawCommandKind::Clear};
    int z_order{0};
    BlendMode blend_mode{BlendMode::Normal};
    std::optional<RectI> clip;
    std::optional<ClearCommand> clear;
    std::optional<SolidRectCommand> solid_rect;
    std::optional<MaskRectCommand> mask_rect;
    std::optional<TexturedQuadCommand> textured_quad;
    std::optional<LineCommand> line;
    std::optional<ShapeCommand> shape;
    std::optional<AdjustmentCommand> adjustment;
};

struct DrawList2D {
    std::vector<DrawCommand2D> commands;
};

} // namespace renderer2d
} // namespace tachyon
