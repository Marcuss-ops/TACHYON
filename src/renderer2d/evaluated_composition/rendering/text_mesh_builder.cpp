#include "tachyon/renderer2d/evaluated_composition/rendering/text_mesh_builder.h"

#include "tachyon/core/scene/evaluator/hashing.h"
#include "tachyon/media/processing/extruder.h"
#include "tachyon/text/layout/layout.h"
#include "tachyon/text/rendering/outline_extractor.h"

#include <algorithm>
#include <cmath>

namespace tachyon::renderer2d {
namespace {

::tachyon::renderer2d::Color to_color(const ColorSpec& color) {
    return {
        static_cast<float>(color.r) / 255.0f,
        static_cast<float>(color.g) / 255.0f,
        static_cast<float>(color.b) / 255.0f,
        static_cast<float>(color.a) / 255.0f
    };
}

math::Matrix4x4 translation_matrix(float x, float y, float z) {
    return math::Matrix4x4::translation({x, y, z});
}

} // namespace

TextMeshBuildResult build_text_extrusion_mesh(
    const scene::EvaluatedLayerState& layer,
    const scene::EvaluatedCompositionState& composition,
    const ::tachyon::text::FontRegistry& font_registry) {

    TextMeshBuildResult result;
    if (layer.type != scene::LayerType::Text || !layer.is_3d || layer.text_content.empty()) {
        return result;
    }

    const ::tachyon::text::Font* font = nullptr;
    if (!layer.font_id.empty()) {
        font = font_registry.find(layer.font_id);
    }
    if (font == nullptr) {
        font = font_registry.default_font();
    }
    if (font == nullptr || !font->is_loaded() || !font->has_freetype_face()) {
        return result;
    }

    ::tachyon::text::TextStyle style;
    style.pixel_size = static_cast<std::uint32_t>(std::max(1.0f, layer.font_size));
    style.fill_color = to_color(layer.fill_color);

    ::tachyon::text::TextBox box;
    box.width = static_cast<std::uint32_t>(std::max(1, layer.width > 0 ? layer.width : composition.width));
    box.height = static_cast<std::uint32_t>(std::max(1, layer.height > 0 ? layer.height : composition.height));
    box.multiline = true;

    ::tachyon::text::TextLayoutOptions layout_options;
    layout_options.word_wrap = true;

    const ::tachyon::text::TextAlignment alignment =
        layer.text_alignment == 1 ? ::tachyon::text::TextAlignment::Center :
        layer.text_alignment == 2 ? ::tachyon::text::TextAlignment::Right :
        ::tachyon::text::TextAlignment::Left;

    const auto layout = ::tachyon::text::layout_text(*font, layer.text_content, style, box, alignment, layout_options);
    if (layout.glyphs.empty()) {
        return result;
    }

    auto mesh = std::make_shared<::tachyon::media::MeshAsset>();
    mesh->path = "inline:text3d:" + layer.id;

    const float depth = std::max(0.01f, layer.extrusion_depth);
    const float bevel = std::max(0.0f, layer.bevel_size);
    std::uint64_t seed = ::tachyon::scene::stable_string_hash(layer.id);
    seed = ::tachyon::scene::hash_combine(seed, ::tachyon::scene::stable_string_hash(layer.text_content));
    seed = ::tachyon::scene::hash_combine(seed, ::tachyon::scene::stable_string_hash(layer.font_id.empty() ? std::string("default") : layer.font_id));
    seed = ::tachyon::scene::hash_combine(seed, static_cast<std::uint64_t>(std::llround(layer.font_size * 1000.0f)));
    seed = ::tachyon::scene::hash_combine(seed, static_cast<std::uint64_t>(std::llround(layer.extrusion_depth * 1000.0f)));
    seed = ::tachyon::scene::hash_combine(seed, static_cast<std::uint64_t>(std::llround(layer.bevel_size * 1000.0f)));
    seed = ::tachyon::scene::hash_combine(seed, static_cast<std::uint64_t>(std::llround(layer.hole_bevel_ratio * 1000.0f)));
    seed = ::tachyon::scene::hash_combine(seed, static_cast<std::uint64_t>(layer.text_alignment));
    seed = ::tachyon::scene::hash_combine(seed, static_cast<std::uint64_t>(box.width));
    seed = ::tachyon::scene::hash_combine(seed, static_cast<std::uint64_t>(box.height));
    result.cache_key = "text3d:" + layer.id + ":" + std::to_string(seed);

    for (const auto& glyph : layout.glyphs) {
        const auto glyph_paths = ::tachyon::text::OutlineExtractor::extract_glyph_outline(
            *font,
            glyph.font_glyph_index,
            static_cast<float>(layout.scale));

        if (glyph_paths.empty()) {
            continue;
        }

        auto submesh = ::tachyon::media::Extruder::extrude_shape(glyph_paths, depth, bevel);
        if (submesh.vertices.empty() || submesh.indices.empty()) {
            continue;
        }

        submesh.transform = translation_matrix(static_cast<float>(glyph.x), static_cast<float>(glyph.y), 0.0f);
        submesh.material.base_color_factor = {
            static_cast<float>(layer.fill_color.r) / 255.0f,
            static_cast<float>(layer.fill_color.g) / 255.0f,
            static_cast<float>(layer.fill_color.b) / 255.0f
        };
        submesh.material.roughness_factor = 0.45f;
        submesh.material.metallic_factor = 0.0f;
        mesh->sub_meshes.push_back(std::move(submesh));
    }

    if (mesh->sub_meshes.empty()) {
        return result;
    }

    result.mesh = std::move(mesh);
    return result;
}

} // namespace tachyon::renderer2d
