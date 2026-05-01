#include "tachyon/text/rendering/outline_extractor.h"
#include "tachyon/text/fonts/core/font.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

namespace tachyon::text {

namespace {

struct DecomposeContext {
    std::vector<scene::EvaluatedShapePath>* paths;
    float scale;
    math::Vector2 last_point;

    static math::Vector2 to_vec2(const FT_Vector* v, float scale) {
        return { static_cast<float>(v->x) / 64.0f * scale, -static_cast<float>(v->y) / 64.0f * scale };
    }
};

int move_to_callback(const FT_Vector* to, void* user) {
    auto* ctx = static_cast<DecomposeContext*>(user);
    scene::EvaluatedShapePath path;
    path.closed = false;

    scene::EvaluatedShapePathPoint pt;
    pt.position = DecomposeContext::to_vec2(to, ctx->scale);
    pt.tangent_in = {0,0};
    pt.tangent_out = {0,0};

    path.points.push_back(pt);
    ctx->paths->push_back(std::move(path));
    ctx->last_point = pt.position;
    return 0;
}

int line_to_callback(const FT_Vector* to, void* user) {
    auto* ctx = static_cast<DecomposeContext*>(user);
    if (ctx->paths->empty()) return 1;
    
    auto& path = ctx->paths->back();
    scene::EvaluatedShapePathPoint pt;
    pt.position = DecomposeContext::to_vec2(to, ctx->scale);
    pt.tangent_in = {0,0};
    pt.tangent_out = {0,0};

    path.points.push_back(pt);
    ctx->last_point = pt.position;
    return 0;
}

int conic_to_callback(const FT_Vector* control, const FT_Vector* to, void* user) {
    auto* ctx = static_cast<DecomposeContext*>(user);
    if (ctx->paths->empty()) return 1;
    
    auto& path = ctx->paths->back();
    math::Vector2 p0 = ctx->last_point;
    math::Vector2 p1_quad = DecomposeContext::to_vec2(control, ctx->scale);
    math::Vector2 p2_quad = DecomposeContext::to_vec2(to, ctx->scale);
    
    // Quadratic (p0, p1_quad, p2_quad) to Cubic (p0, c1, c2, p2_quad)
    math::Vector2 c1 = p0 + (p1_quad - p0) * (2.0f / 3.0f);
    math::Vector2 c2 = p2_quad + (p1_quad - p2_quad) * (2.0f / 3.0f);
    
    // In our ShapePoint format:
    // The PREVIOUS point (last_point) gets a tangent_out = c1 - p0
    // This NEW point gets a tangent_in = c2 - p2_quad
    if (!path.points.empty()) {
        path.points.back().tangent_out = c1 - p0;
    }

    scene::EvaluatedShapePathPoint pt;
    pt.position = p2_quad;
    pt.tangent_in = c2 - p2_quad;
    pt.tangent_out = {0,0};

    path.points.push_back(pt);
    ctx->last_point = pt.position;
    return 0;
}

int cubic_to_callback(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user) {
    auto* ctx = static_cast<DecomposeContext*>(user);
    if (ctx->paths->empty()) return 1;
    
    auto& path = ctx->paths->back();
    math::Vector2 p0 = ctx->last_point;
    math::Vector2 c1 = DecomposeContext::to_vec2(control1, ctx->scale);
    math::Vector2 c2 = DecomposeContext::to_vec2(control2, ctx->scale);
    math::Vector2 p_to = DecomposeContext::to_vec2(to, ctx->scale);
    
    if (!path.points.empty()) {
        path.points.back().tangent_out = c1 - p0;
    }

    scene::EvaluatedShapePathPoint pt;
    pt.position = p_to;
    pt.tangent_in = c2 - p_to;
    pt.tangent_out = {0,0};

    path.points.push_back(pt);
    ctx->last_point = pt.position;
    return 0;
}

} // namespace

std::vector<scene::EvaluatedShapePath> OutlineExtractor::extract_glyph_outline(
    const Font& font,
    std::uint32_t glyph_index,
    float scale) 
{
    std::vector<scene::EvaluatedShapePath> paths;
    if (!font.has_freetype_face()) return paths;

    FT_Face face = static_cast<FT_Face>(font.freetype_face());
    if (FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_BITMAP | FT_LOAD_IGNORE_TRANSFORM)) {
        return paths;
    }

    DecomposeContext ctx;
    ctx.paths = &paths;
    ctx.scale = scale;

    FT_Outline_Funcs funcs;
    funcs.move_to = move_to_callback;
    funcs.line_to = line_to_callback;
    funcs.conic_to = conic_to_callback;
    funcs.cubic_to = cubic_to_callback;
    funcs.shift = 0;
    funcs.delta = 0;

    FT_Outline_Decompose(&face->glyph->outline, &funcs, &ctx);

    // Close all paths (FreeType outlines are implicitly closed)
    for (auto& path : paths) {
        if (path.points.size() > 1) {
            path.closed = true;
        }
    }

    return paths;
}

} // namespace tachyon::text
