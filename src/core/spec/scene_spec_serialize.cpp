#include "tachyon/core/spec/scene_spec_serialize.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include "tachyon/core/spec/scene_spec_audio.h"
#include "tachyon/text/fonts/management/font_manifest.h"
#include "tachyon/runtime/cache/cache_key_builder.h"
#include <fstream>
#include <sstream>

// Layer serialization is now in serialization/layer_serialize.cpp
// json serialize_layer(const LayerSpec& layer); // Declared in layer_serialize.cpp

using json = nlohmann::json;

namespace tachyon {

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------

json serialize_composition(const CompositionSpec& comp) {
    json j;
    j["id"] = comp.id;
    j["name"] = comp.name;
    j["width"] = comp.width;
    j["height"] = comp.height;
    j["duration"] = comp.duration;
    j["frame_rate"] = { {"numerator", comp.frame_rate.numerator}, {"denominator", comp.frame_rate.denominator} };
    if (comp.fps.has_value()) j["fps"] = *comp.fps;
    if (comp.background.has_value()) j["background"] = *comp.background;
    if (comp.environment_path.has_value()) j["environment"] = *comp.environment_path;

    j["layers"] = json::array();
    for (const auto& layer : comp.layers) {
        j["layers"].push_back(serialize_layer(layer));
    }

    if (!comp.audio_tracks.empty()) {
        j["audio_tracks"] = json::array();
        for (const auto& track : comp.audio_tracks) {
            json t;
            t["id"] = track.id;
            t["source_path"] = track.source_path;
            t["volume"] = track.volume;
            t["pan"] = track.pan;
            t["start_offset_seconds"] = track.start_offset_seconds;
            j["audio_tracks"].push_back(t);
        }
    }

    if (!comp.camera_cuts.empty()) {
        j["camera_cuts"] = json::array();
        for (const auto& cut : comp.camera_cuts) {
            json c;
            c["camera_id"] = cut.camera_id;
            c["start_seconds"] = cut.start_seconds;
            c["end_seconds"] = cut.end_seconds;
            j["camera_cuts"].push_back(c);
        }
    }

    if (!comp.variable_decls.empty()) {
        j["variable_decls"] = json::array();
        for (const auto& decl : comp.variable_decls) {
            json d;
            d["name"] = decl.name;
            d["type"] = decl.type;
            j["variable_decls"].push_back(d);
        }
    }

    // Serialize input_props
    if (!comp.input_props.empty()) {
        json ip = json::object();
        for (const auto& [key, val] : comp.input_props) {
            ip[key] = val;
        }
        j["input_props"] = ip;
    }

    // Serialize components
    if (!comp.components.empty()) {
        j["components"] = json::array();
        for (const auto& comp_spec : comp.components) {
            json c;
            c["id"] = comp_spec.id;
            c["name"] = comp_spec.name;
            if (!comp_spec.params.empty()) {
                json params = json::array();
                for (const auto& param : comp_spec.params) {
                    json p;
                    p["name"] = param.name;
                    p["type"] = param.type;
                    params.push_back(p);
                }
                c["params"] = params;
            }
            // TODO: serialize layers inside component
            j["components"].push_back(c);
        }
    }

    // Serialize component instances
    if (!comp.component_instances.empty()) {
        j["component_instances"] = json::array();
        for (const auto& inst : comp.component_instances) {
            json i;
            i["component_id"] = inst.component_id;
            i["instance_id"] = inst.instance_id;
            if (!inst.param_values.empty()) {
                json pv = json::object();
                for (const auto& [key, val] : inst.param_values) {
                    pv[key] = json::parse(val);
                }
                i["param_values"] = pv;
            }
            j["component_instances"].push_back(i);
        }
    }

    return j;
}

json serialize_scene_spec(const SceneSpec& scene) {
    json j;
    j["version"] = scene.version;
    j["spec_version"] = scene.spec_version;
    j["project"] = {
        {"id", scene.project.id},
        {"name", scene.project.name},
        {"authoring_tool", scene.project.authoring_tool}
    };
    if (scene.project.root_seed.has_value()) {
        j["project"]["root_seed"] = *scene.project.root_seed;
    }

    j["compositions"] = json::array();
    for (const auto& comp : scene.compositions) {
        j["compositions"].push_back(serialize_composition(comp));
    }

    if (!scene.assets.empty()) {
        j["assets"] = json::array();
        for (const auto& asset : scene.assets) {
            json a;
            a["id"] = asset.id;
            a["type"] = asset.type;
            a["source"] = asset.source;
            if (!asset.path.empty()) a["path"] = asset.path;
            if (asset.alpha_mode.has_value() && !asset.alpha_mode->empty()) a["alpha_mode"] = *asset.alpha_mode;
            j["assets"].push_back(a);
        }
    }

    return j;
}

// ---------------------------------------------------------------------------
// Merkle Tree Hashing
// ---------------------------------------------------------------------------

std::uint64_t compute_layer_hash(const LayerSpec& layer) {
    CacheKeyBuilder builder;
    builder.add_string(serialize_layer(layer).dump());
    return builder.finish();
}

std::uint64_t compute_composition_hash(const CompositionSpec& comp) {
    CacheKeyBuilder builder;
    // Note: this hashes the serialized composition, which includes its layers' serialized data.
    // In a fully optimized Merkle tree we would only hash the comp's properties + the children's spec_hashes,
    // but the JSON serialization is perfectly deterministic and fast enough for our current scale.
    builder.add_string(serialize_composition(comp).dump());
    return builder.finish();
}

std::uint64_t compute_scene_hash(const SceneSpec& scene) {
    CacheKeyBuilder builder;
    builder.add_string(serialize_scene_spec(scene).dump());
    return builder.finish();
}

} // namespace tachyon

