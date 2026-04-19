#pragma once

#include "tachyon/core/math/vector2.h"
#include "tachyon/renderer2d/framebuffer.h"
#include "tachyon/renderer2d/texture_handle.h"

#include <optional>
#include <vector>

namespace tachyon {
namespace renderer2d {

enum class DrawCommandKind {
    Clear,
    SolidRect,
    MaskRect,
    TexturedQuad,
    Line
};

enum class BlendMode {
    Normal,
    Additive,
    Multiply,
    Screen
};

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
};

struct DrawList2D {
    std::vector<DrawCommand2D> commands;
};

} // namespace renderer2d
} // namespace tachyon
