#include "tachyon/renderer2d/backend/vulkan_compute_backend.h"
#include "tachyon/renderer2d/backend/compute_backend.h"
#include <cstring>
#include <algorithm>
#include <cmath>
#include <vector>

namespace tachyon::renderer2d {

// ---------------------------------------------------------------------------
// CPU backend (always available)
// ---------------------------------------------------------------------------

class CPUComputeBackend : public ComputeBackend {
public:
    BackendType type()  const override { return BackendType::CPU; }
    std::string name()  const override { return "CPU (Reference)"; }
    bool is_available() const override { return true; }
    BackendCaps caps()  const override { return {true, true, true, false}; }

    void* upload(const SurfaceRGBA& surface) override {
        size_t bytes = surface.width() * surface.height() * 4 * sizeof(float);
        float* buf = new float[surface.width() * surface.height() * 4];
        std::memcpy(buf, surface.pixels().data(), bytes);
        auto* h = new CpuHandle{buf, surface.width(), surface.height()};
        return h;
    }

    void download(void* device_ptr, SurfaceRGBA& surface) override {
        auto* h = static_cast<CpuHandle*>(device_ptr);
        size_t bytes = h->w * h->h * 4 * sizeof(float);
        std::memcpy(surface.mutable_pixels().data(), h->data, bytes);
    }

    void free_device_memory(void* device_ptr) override {
        auto* h = static_cast<CpuHandle*>(device_ptr);
        delete[] h->data;
        delete h;
    }

    void resize(void* src, uint32_t sw, uint32_t sh,
                void* dst, uint32_t dw, uint32_t dh) override {
        auto* s = static_cast<CpuHandle*>(src);
        auto* d = static_cast<CpuHandle*>(dst);
        float x_ratio = (float)sw / dw;
        float y_ratio = (float)sh / dh;
        for (uint32_t y = 0; y < dh; ++y) {
            for (uint32_t x = 0; x < dw; ++x) {
                // Bilinear
                float fx = x * x_ratio, fy = y * y_ratio;
                uint32_t x0 = (uint32_t)fx, y0 = (uint32_t)fy;
                uint32_t x1 = std::min(x0+1, sw-1), y1 = std::min(y0+1, sh-1);
                float tx = fx - x0, ty = fy - y0;
                for (int c = 0; c < 4; ++c) {
                    float v00 = s->data[(y0*sw+x0)*4+c];
                    float v10 = s->data[(y0*sw+x1)*4+c];
                    float v01 = s->data[(y1*sw+x0)*4+c];
                    float v11 = s->data[(y1*sw+x1)*4+c];
                    d->data[(y*dw+x)*4+c] = (v00*(1-tx)+v10*tx)*(1-ty)+(v01*(1-tx)+v11*tx)*ty;
                }
            }
        }
        d->w = dw; d->h = dh;
    }

    void color_matrix(void* device_ptr, uint32_t w, uint32_t h,
                      const float m[9]) override {
        auto* hnd = static_cast<CpuHandle*>(device_ptr);
        for (uint32_t i = 0; i < w * h; ++i) {
            float r = hnd->data[i*4], g = hnd->data[i*4+1], b = hnd->data[i*4+2];
            hnd->data[i*4+0] = r*m[0] + g*m[1] + b*m[2];
            hnd->data[i*4+1] = r*m[3] + g*m[4] + b*m[5];
            hnd->data[i*4+2] = r*m[6] + g*m[7] + b*m[8];
        }
    }

    void gaussian_blur(void* device_ptr, uint32_t w, uint32_t h,
                       float sigma_x, float sigma_y) override {
        auto* hnd = static_cast<CpuHandle*>(device_ptr);
        // Separable Gaussian via two 1D passes
        auto make_kernel = [](float sigma, int& radius) {
            radius = std::max(1, (int)std::ceil(sigma * 3.0f));
            std::vector<float> k(radius*2+1);
            float sum = 0;
            for (int i = -radius; i <= radius; ++i) {
                k[i+radius] = std::exp(-(float)(i*i)/(2*sigma*sigma));
                sum += k[i+radius];
            }
            for (auto& v : k) v /= sum;
            return k;
        };

        int rx, ry;
        auto kx = make_kernel(sigma_x, rx);
        auto ky = make_kernel(sigma_y, ry);
        std::vector<float> tmp(w * h * 4);

        // Horizontal pass
        for (uint32_t y = 0; y < h; ++y)
            for (uint32_t x = 0; x < w; ++x)
                for (int c = 0; c < 4; ++c) {
                    float acc = 0;
                    for (int d = -rx; d <= rx; ++d) {
                        int sx = std::clamp((int)x+d, 0, (int)w-1);
                        acc += hnd->data[(y*w+sx)*4+c] * kx[d+rx];
                    }
                    tmp[(y*w+x)*4+c] = acc;
                }

        // Vertical pass back into hnd
        for (uint32_t y = 0; y < h; ++y)
            for (uint32_t x = 0; x < w; ++x)
                for (int c = 0; c < 4; ++c) {
                    float acc = 0;
                    for (int d = -ry; d <= ry; ++d) {
                        int sy = std::clamp((int)y+d, 0, (int)h-1);
                        acc += tmp[(sy*w+x)*4+c] * ky[d+ry];
                    }
                    hnd->data[(y*w+x)*4+c] = acc;
                }
    }

    void apply_lut3d(void* device_ptr, uint32_t w, uint32_t h,
                     const float* lut, uint32_t lut_size) override {
        auto* hnd = static_cast<CpuHandle*>(device_ptr);
        const float scale = (float)(lut_size - 1);
        for (uint32_t i = 0; i < w * h; ++i) {
            float r = std::clamp(hnd->data[i*4+0], 0.0f, 1.0f) * scale;
            float g = std::clamp(hnd->data[i*4+1], 0.0f, 1.0f) * scale;
            float b = std::clamp(hnd->data[i*4+2], 0.0f, 1.0f) * scale;
            uint32_t ri = (uint32_t)r, gi = (uint32_t)g, bi = (uint32_t)b;
            uint32_t ri1 = std::min(ri+1, lut_size-1);
            uint32_t gi1 = std::min(gi+1, lut_size-1);
            uint32_t bi1 = std::min(bi+1, lut_size-1);
            float fr = r-ri, fg = g-gi, fb = b-bi;
            // Trilinear interpolation
            auto idx = [&](uint32_t rr, uint32_t gg, uint32_t bb) {
                return (rr * lut_size * lut_size + gg * lut_size + bb) * 3;
            };
            for (int c = 0; c < 3; ++c) {
                float v000 = lut[idx(ri, gi, bi)+c],   v001 = lut[idx(ri, gi, bi1)+c];
                float v010 = lut[idx(ri, gi1, bi)+c],  v011 = lut[idx(ri, gi1, bi1)+c];
                float v100 = lut[idx(ri1, gi, bi)+c],  v101 = lut[idx(ri1, gi, bi1)+c];
                float v110 = lut[idx(ri1, gi1, bi)+c], v111 = lut[idx(ri1, gi1, bi1)+c];
                float v00 = v000*(1-fb)+v001*fb, v01 = v010*(1-fb)+v011*fb;
                float v10 = v100*(1-fb)+v101*fb, v11 = v110*(1-fb)+v111*fb;
                float v0 = v00*(1-fg)+v01*fg, v1 = v10*(1-fg)+v11*fg;
                hnd->data[i*4+c] = v0*(1-fr)+v1*fr;
            }
        }
    }

private:
    struct CpuHandle { float* data; uint32_t w, h; };
};

// ---------------------------------------------------------------------------
// ComputeBackend convenience methods (default CPU round-trip)
// ---------------------------------------------------------------------------
void ComputeBackend::blur_surface_inplace(SurfaceRGBA& surface, float sigma) {
    void* dev = upload(surface);
    gaussian_blur(dev, surface.width(), surface.height(), sigma, sigma);
    download(dev, surface);
    free_device_memory(dev);
}

void ComputeBackend::color_matrix_inplace(SurfaceRGBA& surface, const float matrix_3x3[9]) {
    void* dev = upload(surface);
    color_matrix(dev, surface.width(), surface.height(), matrix_3x3);
    download(dev, surface);
    free_device_memory(dev);
}

// ---------------------------------------------------------------------------
// Factory functions
// ---------------------------------------------------------------------------
std::unique_ptr<ComputeBackend> create_cpu_backend() {
    return std::make_unique<CPUComputeBackend>();
}

std::unique_ptr<ComputeBackend> create_vulkan_backend() {
#ifdef TACHYON_VULKAN
    auto b = std::make_unique<VulkanComputeBackend>();
    if (b->is_available()) return b;
#endif
    return nullptr;
}

std::unique_ptr<ComputeBackend> create_best_backend() {
    auto vk = create_vulkan_backend();
    if (vk) return vk;
    return create_cpu_backend();
}

} // namespace tachyon::renderer2d
