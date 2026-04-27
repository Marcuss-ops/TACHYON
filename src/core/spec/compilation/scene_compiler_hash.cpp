#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/runtime/cache/cache_key_builder.h"
#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <nlohmann/json.hpp>

namespace tachyon {

void add_string(CacheKeyBuilder& builder, const std::string& value) {
    builder.add_string(std::string_view{value});
}

template <typename T>
void add_json(CacheKeyBuilder& builder, const T& value) {
    builder.add_string(nlohmann::json(value).dump());
}

std::uint64_t hash_scene_spec(const SceneSpec& scene, const DeterminismContract& contract) {
    CacheKeyBuilder builder;
    builder.add_u64(contract.fingerprint());
    
    // O(1) root hash check! The scene.spec_hash represents the entire Merkle Tree
    // pre-computed during JSON parsing or mutation.
    builder.add_u64(scene.spec_hash);
    
    return builder.finish();
}

std::uint64_t hash_scene_structure(const SceneSpec& scene) {
    CacheKeyBuilder builder;
    builder.add_u64(static_cast<std::uint64_t>(scene.compositions.size()));
    for (const auto& composition : scene.compositions) {
        add_string(builder, composition.id);
        builder.add_u64(static_cast<std::uint64_t>(composition.layers.size()));
        for (const auto& layer : composition.layers) {
            add_string(builder, layer.id);
            add_string(builder, layer.type);
            builder.add_bool(layer.parent.has_value());
            if (layer.parent.has_value()) add_string(builder, *layer.parent);
            builder.add_bool(layer.precomp_id.has_value());
            if (layer.precomp_id.has_value()) add_string(builder, *layer.precomp_id);
            builder.add_bool(layer.track_matte_layer_id.has_value());
            if (layer.track_matte_layer_id.has_value()) add_string(builder, *layer.track_matte_layer_id);
        }
    }
    return builder.finish();
}

// Explicit template instantiation for types used in scene_compiler.cpp
template void add_json<AnimatedScalarSpec>(CacheKeyBuilder& builder, const AnimatedScalarSpec& value);
template void add_json<AnimatedVector2Spec>(CacheKeyBuilder& builder, const AnimatedVector2Spec& value);
template void add_json<TextAnimatorSpec>(CacheKeyBuilder& builder, const TextAnimatorSpec& value);
template void add_json<TextHighlightSpec>(CacheKeyBuilder& builder, const TextHighlightSpec& value);

} // namespace tachyon
