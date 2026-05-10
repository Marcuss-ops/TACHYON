#include "tachyon/renderer2d/backend/vulkan_compute_backend.h"
#include <cstring>
#include <algorithm>
#include <cmath>
#include <vector>

namespace tachyon::renderer2d {

#ifdef TACHYON_VULKAN

// VulkanContext DTOR – clean up all Vulkan objects
VulkanContext::~VulkanContext() {
    if (device != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, command_pool, nullptr);
        vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
        vkDestroyDevice(device, nullptr);
    }
    if (instance != VK_NULL_HANDLE)
        vkDestroyInstance(instance, nullptr);
}

VulkanComputeBackend::VulkanComputeBackend() {
    m_ctx = std::make_unique<VulkanContext>();
    if (!init_instance())       return;
    if (!pick_physical_device()) return;
    if (!create_device())       return;
    if (!create_command_pool()) return;
    if (!create_descriptor_pool()) return;
    // build_pipelines() requires SPIR-V blobs – deferred until first use
    m_ctx->valid = true;
}

VulkanComputeBackend::~VulkanComputeBackend() {
    if (m_ctx && m_ctx->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_ctx->device);
        destroy_pipeline(m_pipeline_resize);
        destroy_pipeline(m_pipeline_color_matrix);
        destroy_pipeline(m_pipeline_blur_h);
        destroy_pipeline(m_pipeline_blur_v);
    }
}

bool VulkanComputeBackend::init_instance() {
    VkApplicationInfo app_info{};
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName   = "Tachyon";
    app_info.apiVersion         = VK_API_VERSION_1_2;

    VkInstanceCreateInfo ci{};
    ci.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &app_info;

    return vkCreateInstance(&ci, nullptr, &m_ctx->instance) == VK_SUCCESS;
}

bool VulkanComputeBackend::pick_physical_device() {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(m_ctx->instance, &count, nullptr);
    if (count == 0) return false;
    std::vector<VkPhysicalDevice> devs(count);
    vkEnumeratePhysicalDevices(m_ctx->instance, &count, devs.data());
    // Pick the first device with a compute queue
    for (auto& d : devs) {
        uint32_t qcount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(d, &qcount, nullptr);
        std::vector<VkQueueFamilyProperties> qprops(qcount);
        vkGetPhysicalDeviceQueueFamilyProperties(d, &qcount, qprops.data());
        for (uint32_t i = 0; i < qcount; ++i) {
            if (qprops[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                m_ctx->physical_device = d;
                m_ctx->compute_queue_family = i;
                return true;
            }
        }
    }
    return false;
}

bool VulkanComputeBackend::create_device() {
    float priority = 1.0f;
    VkDeviceQueueCreateInfo qci{};
    qci.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueFamilyIndex = m_ctx->compute_queue_family;
    qci.queueCount       = 1;
    qci.pQueuePriorities = &priority;

    VkDeviceCreateInfo dci{};
    dci.sType            = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos    = &qci;

    if (vkCreateDevice(m_ctx->physical_device, &dci, nullptr, &m_ctx->device) != VK_SUCCESS)
        return false;
    vkGetDeviceQueue(m_ctx->device, m_ctx->compute_queue_family, 0, &m_ctx->compute_queue);
    return true;
}

bool VulkanComputeBackend::create_command_pool() {
    VkCommandPoolCreateInfo ci{};
    ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    ci.queueFamilyIndex = m_ctx->compute_queue_family;
    ci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    return vkCreateCommandPool(m_ctx->device, &ci, nullptr, &m_ctx->command_pool) == VK_SUCCESS;
}

bool VulkanComputeBackend::create_descriptor_pool() {
    VkDescriptorPoolSize ps{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 64};
    VkDescriptorPoolCreateInfo ci{};
    ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ci.maxSets       = 16;
    ci.poolSizeCount = 1;
    ci.pPoolSizes    = &ps;
    return vkCreateDescriptorPool(m_ctx->device, &ci, nullptr, &m_ctx->descriptor_pool) == VK_SUCCESS;
}

VulkanBuffer* VulkanComputeBackend::alloc_buffer(VkDeviceSize bytes, uint32_t w, uint32_t h) {
    auto buf = std::make_unique<VulkanBuffer>();
    buf->size = bytes; buf->width = w; buf->height = h;

    VkBufferCreateInfo bci{};
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size  = bytes;
    bci.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(m_ctx->device, &bci, nullptr, &buf->buffer);

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(m_ctx->device, buf->buffer, &req);

    VkPhysicalDeviceMemoryProperties memprops;
    vkGetPhysicalDeviceMemoryProperties(m_ctx->physical_device, &memprops);

    uint32_t mem_type = UINT32_MAX;
    VkMemoryPropertyFlags wanted = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for (uint32_t i = 0; i < memprops.memoryTypeCount; ++i)
        if ((req.memoryTypeBits & (1u << i)) &&
            (memprops.memoryTypes[i].propertyFlags & wanted) == wanted)
            { mem_type = i; break; }

    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    ai.allocationSize  = req.size;
    ai.memoryTypeIndex = mem_type;
    vkAllocateMemory(m_ctx->device, &ai, nullptr, &buf->memory);
    vkBindBufferMemory(m_ctx->device, buf->buffer, buf->memory, 0);
    vkMapMemory(m_ctx->device, buf->memory, 0, bytes, 0, &buf->mapped);
    return buf.release();
}

void* VulkanComputeBackend::upload(const SurfaceRGBA& surface) {
    size_t bytes = surface.width() * surface.height() * 4 * sizeof(float);
    auto* buf = alloc_buffer(bytes, surface.width(), surface.height());
    std::memcpy(buf->mapped, surface.pixels().data(), bytes);
    return buf;
}

void VulkanComputeBackend::download(void* device_ptr, SurfaceRGBA& surface) {
    auto* buf = static_cast<VulkanBuffer*>(device_ptr);
    size_t bytes = buf->width * buf->height * 4 * sizeof(float);
    std::memcpy(surface.mutable_pixels().data(), buf->mapped, bytes);
}

void VulkanComputeBackend::free_device_memory(void* device_ptr) {
    std::unique_ptr<VulkanBuffer> buf(static_cast<VulkanBuffer*>(device_ptr));
    if (buf) {
        vkUnmapMemory(m_ctx->device, buf->memory);
        vkDestroyBuffer(m_ctx->device, buf->buffer, nullptr);
        vkFreeMemory(m_ctx->device, buf->memory, nullptr);
    }
}

void VulkanComputeBackend::destroy_pipeline(VulkanPipeline& p) {
    if (!m_ctx || m_ctx->device == VK_NULL_HANDLE) return;
    if (p.pipeline  != VK_NULL_HANDLE) vkDestroyPipeline(m_ctx->device, p.pipeline, nullptr);
    if (p.layout    != VK_NULL_HANDLE) vkDestroyPipelineLayout(m_ctx->device, p.layout, nullptr);
    if (p.ds_layout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(m_ctx->device, p.ds_layout, nullptr);
    if (p.shader    != VK_NULL_HANDLE) vkDestroyShaderModule(m_ctx->device, p.shader, nullptr);
    p = {};
}

// Compute ops: fall through to CPU for now (SPIR-V shaders added in Phase 1b)
// These are correct implementations via the CPU path inside the GPU buffer memory
// (which is HOST_VISIBLE|COHERENT, so CPU can access it directly).
void VulkanComputeBackend::resize(void* src, uint32_t sw, uint32_t sh,
                                   void* dst, uint32_t dw, uint32_t dh) {
    if (!m_ctx->valid) return;
    
    // If real compute pipeline is available, use it
    if (m_pipeline_resize.pipeline != VK_NULL_HANDLE) {
        auto* s = static_cast<VulkanBuffer*>(src);
        auto* d = static_cast<VulkanBuffer*>(dst);
        
        auto cmd = begin_commands();
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_resize.pipeline);
        
        // Push constants: sw, sh, dw, dh
        uint32_t push[4] = {sw, sh, dw, dh};
        vkCmdPushConstants(cmd, m_pipeline_resize.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 16, push);
        
        // --- Descriptor Set Update ---
        // In a production engine, we would use a ring-buffer of descriptor sets
        // and update them via vkUpdateDescriptorSets.
        VkDescriptorBufferInfo b_in{}, b_out{};
        b_in.buffer = s->buffer;  b_in.range = VK_WHOLE_SIZE;
        b_out.buffer = d->buffer; b_out.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet writes[2] = {};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[0].pBufferInfo = &b_in;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstBinding = 1;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].pBufferInfo = &b_out;

        // Note: For brevity, we assume the pipeline's descriptor set is pre-allocated
        // and just updated here for the specific buffers.
        // vkUpdateDescriptorSets(m_ctx->device, 2, writes, 0, nullptr);
        // vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_resize.layout, 0, 1, &m_pipeline_resize.ds_set, 0, nullptr);
        
        vkCmdDispatch(cmd, (dw + 15) / 16, (dh + 15) / 16, 1);
        
        submit_and_wait(cmd);
        d->width = dw; d->height = dh;
        return;
    }

    // Fallback: CPU path on mapped memory (current implementation)
    auto* s = static_cast<VulkanBuffer*>(src);
    auto* d = static_cast<VulkanBuffer*>(dst);
    float xr = (float)sw/dw, yr = (float)sh/dh;
    auto* sp = static_cast<float*>(s->mapped);
    auto* dp = static_cast<float*>(d->mapped);
    for (uint32_t y = 0; y < dh; ++y) {
        for (uint32_t x = 0; x < dw; ++x) {
            float fx = x*xr, fy = y*yr;
            uint32_t x0=(uint32_t)fx, y0=(uint32_t)fy;
            uint32_t x1=std::min(x0+1,sw-1), y1=std::min(y0+1,sh-1);
            float tx=fx-x0, ty=fy-y0;
            for (int c = 0; c < 4; ++c) {
                float v = (sp[(y0*sw+x0)*4+c]*(1-tx)+sp[(y0*sw+x1)*4+c]*tx)*(1-ty)
                         +(sp[(y1*sw+x0)*4+c]*(1-tx)+sp[(y1*sw+x1)*4+c]*tx)*ty;
                dp[(y*dw+x)*4+c] = v;
            }
        }
    }
    d->width = dw; d->height = dh;
}

void VulkanComputeBackend::color_matrix(void* device_ptr, uint32_t w, uint32_t h,
                                         const float m[9]) {
    if (!m_ctx->valid) return;

    if (m_pipeline_color_matrix.pipeline != VK_NULL_HANDLE) {
        auto* s = static_cast<VulkanBuffer*>(device_ptr);
        auto cmd = begin_commands();
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_color_matrix.pipeline);
        
        // Push constants: w, h, matrix[9] (44 bytes total)
        struct Push { uint32_t w, h; float m[9]; } push = {w, h};
        std::memcpy(push.m, m, sizeof(float)*9);
        vkCmdPushConstants(cmd, m_pipeline_color_matrix.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 44, &push);
        
        // Descriptor Set Update (In-place storage buffer)
        VkDescriptorBufferInfo b_info{};
        b_info.buffer = s->buffer; b_info.range = VK_WHOLE_SIZE;
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pBufferInfo = &b_info;
        // vkUpdateDescriptorSets(m_ctx->device, 1, &write, 0, nullptr);
        
        vkCmdDispatch(cmd, (w + 15) / 16, (h + 15) / 16, 1);
        submit_and_wait(cmd);
        return;
    }

    auto* p = static_cast<float*>(static_cast<VulkanBuffer*>(device_ptr)->mapped);
    for (uint32_t i = 0; i < w*h; ++i) {
        float r=p[i*4], g=p[i*4+1], b=p[i*4+2];
        p[i*4+0]=r*m[0]+g*m[1]+b*m[2];
        p[i*4+1]=r*m[3]+g*m[4]+b*m[5];
        p[i*4+2]=r*m[6]+g*m[7]+b*m[8];
    }
}

void VulkanComputeBackend::gaussian_blur(void* device_ptr, uint32_t w, uint32_t h,
                                          float sx, float sy) {
    // Temporarily wrap in a CPUComputeBackend-style handle for the separable kernel
    auto* vbuf = static_cast<VulkanBuffer*>(device_ptr);
    auto* data = static_cast<float*>(vbuf->mapped);
    auto make_k = [](float sigma, int& r) {
        r = std::max(1,(int)std::ceil(sigma*3));
        std::vector<float> k(r*2+1); float s=0;
        for (int i=-r;i<=r;++i) { k[i+r]=std::exp(-(float)(i*i)/(2*sigma*sigma)); s+=k[i+r]; }
        for (auto& v:k) v/=s; return k;
    };
    int rx, ry; auto kx=make_k(sx,rx); auto ky=make_k(sy,ry);
    std::vector<float> tmp(w*h*4);
    for (uint32_t y=0;y<h;++y) for (uint32_t x=0;x<w;++x) for (int c=0;c<4;++c) {
        float acc=0;
        for (int d=-rx;d<=rx;++d) { int s=std::clamp((int)x+d,0,(int)w-1); acc+=data[(y*w+s)*4+c]*kx[d+rx]; }
        tmp[(y*w+x)*4+c]=acc;
    }
    for (uint32_t y=0;y<h;++y) for (uint32_t x=0;x<w;++x) for (int c=0;c<4;++c) {
        float acc=0;
        for (int d=-ry;d<=ry;++d) { int s=std::clamp((int)y+d,0,(int)h-1); acc+=tmp[(s*w+x)*4+c]*ky[d+ry]; }
        data[(y*w+x)*4+c]=acc;
    }
}

VkShaderModule VulkanComputeBackend::compile_spv(const uint32_t* spv_data, size_t spv_size) {
    VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    ci.codeSize = spv_size;
    ci.pCode = spv_data;
    VkShaderModule mod;
    if (vkCreateShaderModule(m_ctx->device, &ci, nullptr, &mod) != VK_SUCCESS)
        return VK_NULL_HANDLE;
    return mod;
}

bool VulkanComputeBackend::build_pipelines() {
    // Descriptor Set Layout: 2 Storage Buffers (Input, Output)
    VkDescriptorSetLayoutBinding bindings[2] = {};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo dsl_ci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    dsl_ci.bindingCount = 2;
    dsl_ci.pBindings = bindings;

    auto create_p = [&](VulkanPipeline& p, const uint32_t* spv, size_t size, uint32_t push_size) {
        if (vkCreateDescriptorSetLayout(m_ctx->device, &dsl_ci, nullptr, &p.ds_layout) != VK_SUCCESS)
            return false;

        VkPushConstantRange pcr{};
        pcr.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pcr.offset = 0;
        pcr.size = push_size;

        VkPipelineLayoutCreateInfo pl_ci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pl_ci.setLayoutCount = 1;
        pl_ci.pSetLayouts = &p.ds_layout;
        if (push_size > 0) {
            pl_ci.pushConstantRangeCount = 1;
            pl_ci.pPushConstantRanges = &pcr;
        }

        if (vkCreatePipelineLayout(m_ctx->device, &pl_ci, nullptr, &p.layout) != VK_SUCCESS)
            return false;

        p.shader = compile_spv(spv, size);
        if (!p.shader) return false;

        VkComputePipelineCreateInfo cp_ci{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        cp_ci.layout = p.layout;
        cp_ci.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        cp_ci.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        cp_ci.stage.module = p.shader;
        cp_ci.stage.pName = "main";

        return vkCreateComputePipelines(m_ctx->device, VK_NULL_HANDLE, 1, &cp_ci, nullptr, &p.pipeline) == VK_SUCCESS;
    };

    // Note: In a production environment, these SPIR-V blobs would be loaded from disk or embedded.
    // For now, we define the structure for Phase 1.
    static const uint32_t dummy_spv[] = {0x07230203, 0}; // Minimal invalid SPIR-V for architecture demonstration
    
    // Resize pipeline (Push constants: sw, sh, dw, dh)
    // create_p(m_pipeline_resize, spv_resize, sizeof(spv_resize), 16);
    
    // Color matrix pipeline (Push constants: w, h, matrix[9])
    // create_p(m_pipeline_color_matrix, spv_color, sizeof(spv_color), 44);

    return true;
}

VkCommandBuffer VulkanComputeBackend::begin_commands() {
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.commandPool        = m_ctx->command_pool;
    ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(m_ctx->device, &ai, &cmd);
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &bi);
    return cmd;
}

void VulkanComputeBackend::submit_and_wait(VkCommandBuffer cmd) {
    vkEndCommandBuffer(cmd);
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1; si.pCommandBuffers = &cmd;
    vkQueueSubmit(m_ctx->compute_queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_ctx->compute_queue);
    vkFreeCommandBuffers(m_ctx->device, m_ctx->command_pool, 1, &cmd);
}

#endif // TACHYON_VULKAN

} // namespace tachyon::renderer2d
