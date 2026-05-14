#include "tachyon/runtime/compiler/scene_compiler_hashing.h"
#include "tachyon/runtime/cache/cache_key_builder.h"

#include <algorithm>

namespace tachyon {

void add_string(CacheKeyBuilder& builder, const std::string& value) {
    builder.add_string(value);
}

namespace {

void hash_transition_internal(CacheKeyBuilder& builder, const LayerTransitionSpec& spec) {
    builder.add_string(spec.transition_id);
    builder.add_f64(spec.duration);
    builder.add_f64(spec.delay);
}

} // namespace

std::uint64_t hash_scene_spec(const SceneSpec& scene, const DeterminismContract& contract) {
    CacheKeyBuilder builder;
    builder.add_string(scene.project.id);
    builder.add_u64(contract.fingerprint());
    
    for (const auto& comp : scene.compositions) {
        builder.add_string(comp.id);
        builder.add_f64(comp.duration);
        
        for (const auto& layer : comp.layers) {
            builder.add_string(layer.identity.id);
            builder.add_u32(static_cast<std::uint32_t>(layer.identity.type));
            builder.add_u32(static_cast<std::uint32_t>(layer.blend_mode));
            
            builder.add_f64(layer.playback.timing.start);
            builder.add_f64(layer.playback.timing.duration);
            
            builder.add_f64(layer.transform.opacity);
            builder.add_u32(layer.transform.width);
            builder.add_u32(layer.transform.height);

            // Hash nested transform
            builder.add_f64(layer.transform.transform.position_x.value_or(0.0));
            builder.add_f64(layer.transform.transform.position_y.value_or(0.0));
            builder.add_f64(layer.transform.transform.scale_x.value_or(100.0));
            builder.add_f64(layer.transform.transform.scale_y.value_or(100.0));
            builder.add_f64(layer.transform.transform.rotation.value_or(0.0));
            builder.add_f64(layer.transform.transform.anchor_point.value.value_or(math::Vector2::zero()).x);
            builder.add_f64(layer.transform.transform.anchor_point.value.value_or(math::Vector2::zero()).y);

            hash_transition_internal(builder, layer.transition_in);
            hash_transition_internal(builder, layer.transition_out);
            
            std::visit([&](auto&& source) {
                using T = std::decay_t<decltype(source)>;
                if constexpr (std::is_same_v<T, MediaSource>) {
                    builder.add_string(source.asset.id);
                } else if constexpr (std::is_same_v<T, PrecompSource>) {
                    builder.add_string(source.precomp_id);
                } else if constexpr (std::is_same_v<T, ProceduralSource>) {
                    builder.add_string(source.kind);
                }
            }, layer.source);
            
            builder.add_u32(static_cast<std::uint32_t>(layer.effects.size()));
            for (const auto& effect : layer.effects) {
                builder.add_string(effect.type);
                builder.add_bool(effect.enabled);
            }
        }
    }
    
    return builder.finish();
}

} // namespace tachyon
