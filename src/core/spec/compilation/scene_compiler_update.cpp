#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "src/core/spec/compilation/compilation_context.h"
#include "src/core/spec/compilation/property_compiler.h"

namespace tachyon {

// Forward declarations from scene_compiler_hash.cpp
std::uint64_t hash_scene_spec(const SceneSpec& scene, const DeterminismContract& contract);
std::uint64_t hash_scene_structure(const SceneSpec& scene);

static bool try_property_only_update(CompiledScene& existing, const SceneSpec& new_spec, std::uint64_t new_hash) {
    if (new_spec.compositions.size() != existing.compositions.size()) return false;

    for (std::size_t c_idx = 0; c_idx < new_spec.compositions.size(); ++c_idx) {
        if (new_spec.compositions[c_idx].layers.size() != existing.compositions[c_idx].layers.size())
            return false;
    }

    CompilationRegistry registry;
    DiagnosticBag ignored;

    for (std::size_t c_idx = 0; c_idx < new_spec.compositions.size(); ++c_idx) {
        const auto& comp_spec = new_spec.compositions[c_idx];
        auto& comp = existing.compositions[c_idx];

        for (std::size_t l_idx = 0; l_idx < comp_spec.layers.size(); ++l_idx) {
            const auto& layer_spec = comp_spec.layers[l_idx];
            auto& compiled_layer = comp.layers[l_idx];

            compiled_layer.in_time    = layer_spec.in_point;
            compiled_layer.out_time   = layer_spec.out_point;
            compiled_layer.start_time = layer_spec.start_time;
            compiled_layer.blend_mode = layer_spec.blend_mode;
            compiled_layer.transition_in = layer_spec.transition_in;
            compiled_layer.transition_out = layer_spec.transition_out;
            compiled_layer.fill_color   = layer_spec.fill_color.value.value_or(ColorSpec{255,255,255,255});
            compiled_layer.stroke_color = layer_spec.stroke_color.value.value_or(ColorSpec{255,255,255,255});
            compiled_layer.text_content = layer_spec.text_content;

            const auto recompile_track = [&](std::size_t idx, const std::string& suffix, const auto& spec, double fallback) {
                if (idx >= compiled_layer.property_indices.size()) return;
                const std::uint32_t track_idx = compiled_layer.property_indices[idx];
                if (track_idx >= existing.property_tracks.size()) return;
                existing.property_tracks[track_idx] = PropertyCompiler::compile_track(
                    registry, suffix, layer_spec.id, spec, fallback, existing.expressions, ignored);
            };

            const auto& ap = layer_spec.transform.anchor_point;
            recompile_track(0, ".opacity",         layer_spec.opacity_property,              layer_spec.opacity);
            recompile_track(1, ".position_x",      layer_spec.transform.position_property,   layer_spec.transform.position_x.value_or(0.0));
            recompile_track(2, ".position_y",      layer_spec.transform.position_property,   layer_spec.transform.position_y.value_or(0.0));
            recompile_track(3, ".scale_x",         layer_spec.transform.scale_property,      layer_spec.transform.scale_x.value_or(1.0));
            recompile_track(4, ".scale_y",         layer_spec.transform.scale_property,      layer_spec.transform.scale_y.value_or(1.0));
            recompile_track(5, ".rotation",        layer_spec.transform.rotation_property,   layer_spec.transform.rotation.value_or(0.0));
            recompile_track(6, ".mask_feather",    layer_spec.mask_feather,                  0.0);
            recompile_track(7, ".anchor_point_x",  ap,  ap.value.has_value() ? ap.value->x : 0.0);
            recompile_track(8, ".anchor_point_y",  ap,  ap.value.has_value() ? ap.value->y : 0.0);
        }
    }

    existing.scene_hash = new_hash;
    return true;
}

bool SceneCompiler::update_compiled_scene(CompiledScene& existing, const SceneSpec& new_spec) const {
    const std::uint64_t new_hash = hash_scene_spec(new_spec, existing.determinism);
    if (new_hash == existing.scene_hash) {
        return true;
    }

    const std::uint64_t new_structure_hash = hash_scene_structure(new_spec);
    if (new_structure_hash == existing.scene_structure_hash &&
        try_property_only_update(existing, new_spec, new_hash)) {
        return true;
    }

    auto result = compile(new_spec);
    if (result.ok()) {
        existing = std::move(*result.value);
        return true;
    }

    return false;
}

} // namespace tachyon
