#pragma once

// Vulkan compute backend for Tachyon.
// Compiled only when TACHYON_VULKAN is defined (set by CMake when Vulkan SDK found).
// The implementation uses vulkan.h directly; no external framework required.
//
// This header is included by cpu_backend.cpp and vulkan_compute_backend.cpp only.
// All other code uses the ComputeBackend abstract interface.

#include "tachyon/renderer2d/backend/compute_backend.h"

#ifdef TACHYON_VULKAN
#include <vulkan/vulkan.h>
#endif

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

namespace tachyon::renderer2d {

#ifdef TACHYON_VULKAN

// RAII wrappers for core Vulkan objects
struct VulkanContext {
    VkInstance       instance{VK_NULL_HANDLE};
    VkPhysicalDevice physical_device{VK_NULL_HANDLE};
    VkDevice         device{VK_NULL_HANDLE};
    VkQueue          compute_queue{VK_NULL_HANDLE};
    uint32_t         compute_queue_family{0};
    VkCommandPool    command_pool{VK_NULL_HANDLE};
    VkDescriptorPool descriptor_pool{VK_NULL_HANDLE};

    bool valid{false};

    ~VulkanContext();
};

// Per-pipeline (one per compute shader)
struct VulkanPipeline {
    VkPipeline            pipeline{VK_NULL_HANDLE};
    VkPipelineLayout      layout{VK_NULL_HANDLE};
    VkDescriptorSetLayout ds_layout{VK_NULL_HANDLE};
    VkShaderModule        shader{VK_NULL_HANDLE};
};

// A device buffer + memory pair
struct VulkanBuffer {
    VkBuffer       buffer{VK_NULL_HANDLE};
    VkDeviceMemory memory{VK_NULL_HANDLE};
    VkDeviceSize   size{0};
    uint32_t       width{0};
    uint32_t       height{0};
    void*          mapped{nullptr};  // persistently mapped (HOST_VISIBLE|COHERENT)
};

class VulkanComputeBackend : public ComputeBackend {
public:
    VulkanComputeBackend();
    ~VulkanComputeBackend() override;

    BackendType type()  const override { return BackendType::Vulkan; }
    std::string name()  const override { return "Vulkan Compute"; }
    bool is_available() const override { return m_ctx && m_ctx->valid; }
    BackendCaps caps()  const override {
        return {.resize=true, .color_matrix=true, .gaussian_blur=true, .lut3d=false};
    }

    void* upload(const SurfaceRGBA& surface) override;
    void  download(void* device_ptr, SurfaceRGBA& surface) override;
    void  free_device_memory(void* device_ptr) override;

    void resize(void* src, uint32_t sw, uint32_t sh,
                void* dst, uint32_t dw, uint32_t dh) override;

    void color_matrix(void* device_ptr, uint32_t w, uint32_t h,
                      const float matrix_3x3[9]) override;

    void gaussian_blur(void* device_ptr, uint32_t w, uint32_t h,
                       float sigma_x, float sigma_y) override;

    void apply_lut3d(void* device_ptr, uint32_t w, uint32_t h,
                     const float* lut, uint32_t lut_size) override;

private:
    std::unique_ptr<VulkanContext>  m_ctx;
    VulkanPipeline                  m_pipeline_resize;
    VulkanPipeline                  m_pipeline_color_matrix;
    VulkanPipeline                  m_pipeline_blur_h;
    VulkanPipeline                  m_pipeline_blur_v;

    bool init_instance();
    bool pick_physical_device();
    bool create_device();
    bool create_command_pool();
    bool create_descriptor_pool();
    bool build_pipelines();

    VulkanBuffer* alloc_buffer(VkDeviceSize bytes, uint32_t w, uint32_t h);
    VkShaderModule compile_spv(const uint32_t* spv_data, size_t spv_size);

    // Execute a single-use command buffer
    VkCommandBuffer begin_commands();
    void submit_and_wait(VkCommandBuffer cmd);

    void destroy_pipeline(VulkanPipeline& p);
};

#endif // TACHYON_VULKAN

} // namespace tachyon::renderer2d
