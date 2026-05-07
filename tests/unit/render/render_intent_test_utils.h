#pragma once

#include "tachyon/render/intent_builder.h"
#include "tachyon/core/scene/state/evaluated_composition_state.h"
#include <memory>
#include <string>

namespace tachyon::test {

/**
 * @brief A fake resource provider for testing RenderIntent building.
 * It tracks the last IDs requested to verify normalization and lookups.
 */
struct FakeResourceProvider final : public render::IResourceProvider {
    std::string last_mesh_id;
    std::string last_texture_id;
    std::string last_deform_id;

    std::shared_ptr<media::MeshAsset> get_mesh(const std::string& id) override {
        last_mesh_id = id;
        return nullptr;
    }

    std::shared_ptr<std::uint8_t[]> get_texture_rgba(const std::string& id) override {
        last_texture_id = id;
        return nullptr;
    }

    std::shared_ptr<const render::IDeformMesh> get_deform(const std::string& id) override {
        last_deform_id = id;
        return nullptr;
    }
};

/**
 * @brief Helper to build a RenderIntent for tests.
 */
inline render::IntentBuildResult build_test_render_intent(
    const scene::EvaluatedCompositionState& state,
    render::IResourceProvider* provider = nullptr) 
{
    static FakeResourceProvider default_provider;
    return render::build_render_intent(state, provider ? provider : &default_provider);
}

/**
 * @brief Helper to build a base EvaluatedCompositionState for tests.
 */
inline scene::EvaluatedCompositionState make_test_composition(const std::string& id = "test_comp") {
    scene::EvaluatedCompositionState state;
    state.composition_id = id;
    state.width = 1920;
    state.height = 1080;
    return state;
}

/**
 * @brief Helper to build a base EvaluatedLayerState for tests.
 */
inline scene::EvaluatedLayerState make_test_layer(const std::string& id = "test_layer") {
    scene::EvaluatedLayerState layer;
    layer.id = id;
    layer.active = true;
    layer.visible = true;
    layer.enabled = true;
    layer.opacity = 1.0f;
    return layer;
}

} // namespace tachyon::test
