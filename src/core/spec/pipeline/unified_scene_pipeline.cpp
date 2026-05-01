#include "tachyon/core/spec/pipeline/unified_scene_pipeline.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/runtime/core/data/compiled_scene.h"
#include "tachyon/core/spec/compilation/compilation_context.h"
#include "tachyon/core/spec/compilation/property_compiler.h"
#include "tachyon/core/spec/scene_spec_serialize.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/core/spec/legacy/json_parse_helpers.h"
#include "tachyon/core/spec/property_spec_serialize_helpers.h"
#include <nlohmann/json.hpp>

namespace nlohmann {
    template <typename T>
    struct adl_serializer<std::optional<T>> {
        static void to_json(json& j, const std::optional<T>& opt) {
            if (opt) j = *opt; else j = nullptr;
        }
    };
}

namespace tachyon::pipeline {

void UnifiedSceneSpecPipeline::ParseLayer(const nlohmann::json& j, LayerSpec& out, const std::string& path, DiagnosticBag& diagnostics) {
    #define TACHYON_FIELD_string(jk, cn) read_string(j, #jk, out.cn);
    #define TACHYON_FIELD_bool(jk, cn) read_bool(j, #jk, out.cn);
    #define TACHYON_FIELD_int(jk, cn) read_number<int>(j, #jk, out.cn);
    #define TACHYON_FIELD_double(jk, cn) read_number<double>(j, #jk, out.cn);
    
    #define TACHYON_FIELD(jk, cn, type, fallback) TACHYON_FIELD_##type(jk, cn)
    
    #define TACHYON_ANIM_PROP_scalar(jk, cn) parse_optional_scalar_property(j, #jk, out.cn, path, diagnostics);
    #define TACHYON_ANIM_PROP_color(jk, cn) parse_optional_color_property(j, #jk, out.cn, path, diagnostics);
    #define TACHYON_ANIM_PROP_vector2(jk, cn) parse_optional_vector_property(j, #jk, out.cn, path, diagnostics);
    
    #define TACHYON_ANIM_PROP(jk, cn, type, fallback) TACHYON_ANIM_PROP_##type(jk, cn)
    
    #define TACHYON_LAYER_ONLY
    #include "tachyon/core/spec/pipeline/scene_schema.def"
    #undef TACHYON_LAYER_ONLY
    
    #undef TACHYON_FIELD_string
    #undef TACHYON_FIELD_bool
    #undef TACHYON_FIELD_int
    #undef TACHYON_FIELD_double
    #undef TACHYON_FIELD
    #undef TACHYON_ANIM_PROP_scalar
    #undef TACHYON_ANIM_PROP_color
    #undef TACHYON_ANIM_PROP_vector2
    #undef TACHYON_ANIM_PROP
}

nlohmann::json UnifiedSceneSpecPipeline::SerializeLayer(const LayerSpec& layer) {
    nlohmann::json j = nlohmann::json::object();
    
    #define TACHYON_FIELD(jk, cn, type, fb) \
        j[#jk] = layer.cn;
        
    #define TACHYON_ANIM_PROP(jk, cn, type, fb) \
        if (!layer.cn.empty()) j[#jk] = layer.cn;
    
    #define TACHYON_LAYER_ONLY
    #include "tachyon/core/spec/pipeline/scene_schema.def"
    #undef TACHYON_LAYER_ONLY
    
    #undef TACHYON_FIELD
    #undef TACHYON_ANIM_PROP
    
    return j;
}

void UnifiedSceneSpecPipeline::CompileLayerProperties(
    const LayerSpec& layer, 
    void* compiled_layer_ptr, 
    void* compiled_scene_ptr,
    void* registry_ptr,
    DiagnosticBag& diagnostics) {
    
    if (!compiled_layer_ptr || !compiled_scene_ptr || !registry_ptr) return;

    auto& compiled_layer = *static_cast<CompiledLayer*>(compiled_layer_ptr);
    auto& compiled = *static_cast<CompiledScene*>(compiled_scene_ptr);
    auto& registry = *static_cast<CompilationRegistry*>(registry_ptr);

    const auto add_track = [&](const std::string& suffix, const auto& spec, double fallback) {
        compiled_layer.property_indices.push_back(static_cast<std::uint32_t>(compiled.property_tracks.size()));
        auto track = PropertyCompiler::compile_track(registry, suffix, layer.id, spec, fallback, compiled.expressions, diagnostics);
        compiled.graph.add_node(track.node.node_id);
        compiled.graph.add_edge(track.node.node_id, compiled_layer.node.node_id, true);
        compiled.property_tracks.push_back(std::move(track));
    };

    #define TACHYON_FIELD(jk, cn, type, fb) 
    
    #define TACHYON_ANIM_PROP_scalar(jk, cn, fb) \
        add_track("." #jk, layer.cn, static_cast<double>(fb));
    
    #define TACHYON_ANIM_PROP_vector2(jk, cn, fb) \
        add_track("." #jk "_x", layer.cn, static_cast<double>(fb.x)); \
        add_track("." #jk "_y", layer.cn, static_cast<double>(fb.y));

    #define TACHYON_ANIM_PROP_color(jk, cn, fb) 
    
    #define TACHYON_ANIM_PROP(jk, cn, type, fb) TACHYON_ANIM_PROP_##type(jk, cn, fb)
    
    #define TACHYON_LAYER_ONLY
    #include "tachyon/core/spec/pipeline/scene_schema.def"
    #undef TACHYON_LAYER_ONLY
    
    #undef TACHYON_FIELD
    #undef TACHYON_ANIM_PROP_scalar
    #undef TACHYON_ANIM_PROP_vector2
    #undef TACHYON_ANIM_PROP_color
    #undef TACHYON_ANIM_PROP
}

} // namespace tachyon::pipeline
