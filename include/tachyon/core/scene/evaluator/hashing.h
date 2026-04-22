#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <string>
#include <cstdint>

namespace tachyon::scene {

std::uint64_t stable_string_hash(const std::string& text);
std::uint64_t hash_combine(std::uint64_t lhs, std::uint64_t rhs);
std::uint64_t make_base_expression_seed(const SceneSpec* scene, const CompositionSpec& composition);
std::uint64_t make_property_expression_seed(
    const SceneSpec* scene,
    const CompositionSpec& composition,
    const LayerSpec& layer,
    const char* property_name);

} // namespace tachyon::scene
