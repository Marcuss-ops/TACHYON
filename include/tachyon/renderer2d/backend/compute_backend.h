#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include <cstdint>
#include <memory>
#include <string>

namespace tachyon::renderer2d {

enum class BackendType { CPU, Vulkan, Metal, CUDA, OpenCL };

// Runtime capability flags – query before calling optional ops.
struct BackendCaps {
    bool resize{false};
    bool color_matrix{false};
    bool gaussian_blur{false};
    bool lut3d{false};
};

class ComputeBackend {
public:
    virtual ~ComputeBackend() = default;

    virtual BackendType type()  const = 0;
    virtual std::string name()  const = 0;
    virtual bool is_available() const = 0;  // false → caller must use CPU fallback
    virtual BackendCaps caps()  const = 0;

    // --- Buffer lifecycle ---
    /**
     * @brief Upload surface pixels to device memory.
     * 
     * This method acts as an allocator and transfers ownership of the allocated 
     * device memory to the caller. The returned opaque handle MUST be freed 
     * by calling free_device_memory().
     * 
     * @param surface Source pixels.
     * @return An opaque device handle (ownership transferred to caller).
     */
    virtual void* upload(const SurfaceRGBA& surface) = 0;

    /**
     * @brief Download device buffer back to an existing surface.
     * 
     * @param device_ptr Opaque handle returned by upload().
     * @param surface Destination surface (must have correct dimensions).
     */
    virtual void download(void* device_ptr, SurfaceRGBA& surface) = 0;

    /**
     * @brief Free device memory allocated by upload().
     * 
     * @param device_ptr Opaque handle returned by upload().
     */
    virtual void free_device_memory(void* device_ptr) = 0;

    // --- Compute operations ---
    // Bilinear resize. src and dst are device pointers.
    virtual void resize(void* src, uint32_t sw, uint32_t sh,
                        void* dst, uint32_t dw, uint32_t dh) = 0;

    // Apply 3x3 colour matrix (row-major) to every pixel. In-place on device_ptr.
    virtual void color_matrix(void* device_ptr, uint32_t w, uint32_t h,
                               const float matrix_3x3[9]) = 0;

    // Separable Gaussian blur. sigma_x / sigma_y in pixels. In-place.
    virtual void gaussian_blur(void* device_ptr, uint32_t w, uint32_t h,
                                float sigma_x, float sigma_y) = 0;

    // Apply 3D LUT (size^3 * 3 floats, row-major R-G-B). In-place.
    virtual void apply_lut3d(void* device_ptr, uint32_t w, uint32_t h,
                              const float* lut, uint32_t lut_size) = 0;

    // --- Convenience: round-trip helpers ---
    // Upload, apply gaussian_blur, download, free.
    void blur_surface_inplace(SurfaceRGBA& surface, float sigma);

    // Upload, apply color_matrix, download, free.
    void color_matrix_inplace(SurfaceRGBA& surface, const float matrix_3x3[9]);
};

// Factory functions
std::unique_ptr<ComputeBackend> create_cpu_backend();
std::unique_ptr<ComputeBackend> create_vulkan_backend();   // returns null if Vulkan unavailable
std::unique_ptr<ComputeBackend> create_best_backend();     // probes Vulkan, falls back to CPU

} // namespace tachyon::renderer2d
