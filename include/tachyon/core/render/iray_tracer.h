#pragma once

#include "tachyon/core/render/scene_3d.h"
#include "tachyon/core/render/aov_buffer.h"
#include <memory>
#include <string>

namespace tachyon {

/**
 * @brief Abstract interface for the 3D RayTracer.
 * Allows the 2D core and runtime to use 3D rendering without a direct 
 * dependency on the renderer3d implementation (Embree, etc.).
 */
class IRayTracer {
public:
    virtual ~IRayTracer() = default;

    /**
     * @brief Builds/updates the internal acceleration structures for the given 3D scene.
     */
    virtual void build_scene(const EvaluatedScene3D& scene) = 0;

    /**
     * @brief Renders the 3D scene into the provided AOV buffer.
     * 
     * @param scene The evaluated 3D scene.
     * @param out_buffer The buffer to receive beauty, depth, and other AOV passes.
     * @param frame_time_seconds The current absolute time.
     * @param frame_duration_seconds The duration of the current frame (for motion blur).
     */
    virtual void render(
        const EvaluatedScene3D& scene,
        AOVBuffer& out_buffer,
        double frame_time_seconds = 0.0,
        double frame_duration_seconds = 0.0) = 0;

    /**
     * @brief Configures quality settings.
     */
    virtual void set_samples_per_pixel(int spp) = 0;
    virtual void set_denoiser_enabled(bool enabled) = 0;

    /**
     * @brief Returns the last error message, if any.
     */
    virtual const std::string& last_error() const = 0;
};

} // namespace tachyon
