#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include "tachyon/core/spec/scene_spec_audio.h"

#include <nlohmann/json.hpp>

namespace tachyon {

using json = nlohmann::json;

// Layer serialization (defined in serialization/layer_serialize.cpp)
json serialize_layer(const LayerSpec& layer);

// Composition and scene serialization (defined in scene_spec_serialize.cpp)
json serialize_composition(const CompositionSpec& comp);
json serialize_scene_spec(const SceneSpec& scene);

// Property serialization helpers (defined in serialization/property_*_serialize.cpp)
json serialize_vector3_property(const AnimatedVector3Spec& prop);
json serialize_color_property(const AnimatedColorSpec& prop);

// Merkle tree hashing
std::uint64_t compute_layer_hash(const LayerSpec& layer);
std::uint64_t compute_composition_hash(const CompositionSpec& comp);
std::uint64_t compute_scene_hash(const SceneSpec& scene);

} // namespace tachyon
