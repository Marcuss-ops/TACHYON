#pragma once

#include "tachyon/core/api.h"
#include "tachyon/core/types/diagnostics.h"
#include "tachyon/render/render_intent.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include <memory>
#include <vector>
#include <string>

namespace tachyon::render {

/**
 * @brief Status codes for RenderIntent building.
 */
enum class IntentBuildStatus {
    Ok,
    Success = Ok,
    PartialSuccess,      ///< Some layers failed to resolve resources
    AssetResolutionError, ///< Critical assets (meshes, textures) could not be resolved
    PolicyViolation,     ///< Strict mode triggered a failure
    InternalError
};

/**
 * @brief Result of a RenderIntent building operation.
 */
struct RenderIntentBuildResult {
    IntentBuildStatus status{IntentBuildStatus::Ok};
    RenderIntent intent;
    DiagnosticBag diagnostics;

    bool has_errors() const {
        return !diagnostics.ok();
    }
};

// Forward declare a generic resource context to avoid renderer deps in header
class IResourceProvider;

/**
 * @brief Utility to resolve logical asset IDs from evaluated state into physical resources.
 * This lives in [tachyon::render] and is the bridge between Eval and Renderer.
 */
TACHYON_API RenderIntentBuildResult build_render_intent(
    const scene::EvaluatedCompositionState& state,
    IResourceProvider* resource_provider);

/**
 * @brief Interface for resolving resources during intent building.
 * Implementations will be provided by specific renderers or a central media manager.
 */
class IResourceProvider {
public:
    virtual ~IResourceProvider() = default;
    
    virtual std::shared_ptr<::tachyon::media::MeshAsset> get_mesh(const std::string& id) = 0;
    virtual std::shared_ptr<std::uint8_t[]> get_texture_rgba(const std::string& id) = 0;
    virtual std::shared_ptr<const IDeformMesh> get_deform(const std::string& id) = 0;
};

} // namespace tachyon::render
