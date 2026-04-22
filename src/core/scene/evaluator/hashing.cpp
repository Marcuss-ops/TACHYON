#include "tachyon/core/scene/evaluator/hashing.h"

namespace tachyon::scene {

std::uint64_t stable_string_hash(const std::string& text) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (unsigned char ch : text) {
        hash ^= static_cast<std::uint64_t>(ch);
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::uint64_t hash_combine(std::uint64_t lhs, std::uint64_t rhs) {
    lhs ^= rhs + 0x9E3779B97F4A7C15ULL + (lhs << 6) + (lhs >> 2);
    return lhs;
}

std::uint64_t make_base_expression_seed(const SceneSpec* scene, const CompositionSpec& composition) {
    std::uint64_t seed = scene && scene->project.root_seed.has_value()
        ? static_cast<std::uint64_t>(*scene->project.root_seed)
        : 0x6D6574616C736565ULL;
    seed = hash_combine(seed, stable_string_hash(composition.id));
    seed = hash_combine(seed, stable_string_hash(composition.name));
    return seed;
}

std::uint64_t make_property_expression_seed(
    const SceneSpec* scene,
    const CompositionSpec& composition,
    const LayerSpec& layer,
    const char* property_name) {
    std::uint64_t seed = make_base_expression_seed(scene, composition);
    seed = hash_combine(seed, stable_string_hash(layer.id));
    seed = hash_combine(seed, stable_string_hash(layer.name));
    seed = hash_combine(seed, stable_string_hash(property_name));
    return seed;
}

} // namespace tachyon::scene
