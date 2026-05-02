// Implementation of RayTracer - render() orchestration only
#include "tachyon/renderer3d/core/ray_tracer.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/math/matrix4x4.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace tachyon::renderer3d {

namespace {

math::Vector2 sample_concentric_disk(float u1, float u2) {
    const float sx = 2.0f * u1 - 1.0f;
    const float sy = 2.0f * u2 - 1.0f;

    if (sx == 0.0f && sy == 0.0f) {
        return {0.0f, 0.0f};
    }

    float r = 0.0f;
    float theta = 0.0f;
    if (std::abs(sx) > std::abs(sy)) {
        r = sx;
        theta = (3.1415926535f / 4.0f) * (sy / sx);
    } else {
        r = sy;
        theta = (3.1415926535f / 2.0f) - (3.1415926535f / 4.0f) * (sx / sy);
    }
    return {r * std::cos(theta), r * std::sin(theta)};
}

} // namespace

RayTracer::RayTracer(media::MediaManager* media_manager) 
    : media_manager_(media_manager), m_last_error("") {
    device_ = rtcNewDevice(nullptr);
    rtcSetDeviceErrorFunction(device_, log_embree_error, this);
#ifdef _WIN32
    // Use the available oidn stub/library
    oidn_device_ = oidn::newDevice();
    if (oidn_device_) {
        oidn_device_.commit();
        oidn_filter_ = oidn_device_.newFilter("RT");
    }
#endif
}

RayTracer::~RayTracer() {
    cleanup_scene();
    if (device_) {
        rtcReleaseDevice(device_);
    }
}

void RayTracer::render(
    const EvaluatedScene3D& scene,
    AOVBuffer& out_buffer,
    const MotionBlurRenderer* motion_blur,
    double frame_time_seconds,
    double frame_duration_seconds) {

    const int width = static_cast<int>(out_buffer.width);
    const int height = static_cast<int>(out_buffer.height);

    if (width <= 0 || height <= 0) {
        return;
    }

    out_buffer.clear();

    if (!scene_ || scene.instances.empty()) {
        return;
    }

    const auto& cam = scene.camera;

    // Use a deterministic seed based on the frame time
    // This ensures identical results when rendering the same frame twice.
    std::uint32_t seed = static_cast<std::uint32_t>(std::hash<double>{}(frame_time_seconds));
    const float time = static_cast<float>(frame_time_seconds);

    // Compute camera vectors
    math::Vector3 cam_forward = (cam.target - cam.position).normalized();
    math::Vector3 cam_right = math::Vector3::cross(cam_forward, cam.up).normalized();
    math::Vector3 cam_up = math::Vector3::cross(cam_right, cam_forward).normalized();
    float fov_rad = cam.fov_y * 3.1415926535f / 180.0f;
    float half_height = std::tan(fov_rad * 0.5f);
    float aspect = static_cast<float>(width) / static_cast<float>(height);

    #pragma omp parallel for collapse(2) schedule(dynamic, 8)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Per-pixel deterministic RNG
            // We mix the global seed with pixel coordinates to ensure unique but repeatable patterns
            std::uint32_t pixel_seed = seed ^ (static_cast<std::uint32_t>(y) * 1973 + static_cast<std::uint32_t>(x) * 9277);
            pixel_seed = (pixel_seed ^ 61) ^ (pixel_seed >> 16);
            pixel_seed *= 9;
            pixel_seed = pixel_seed ^ (pixel_seed >> 4);
            pixel_seed *= 0x27d4eb2d;
            pixel_seed = pixel_seed ^ (pixel_seed >> 15);
            std::mt19937 pixel_rng(pixel_seed);

            math::Vector3 pixel_color = {0.0f, 0.0f, 0.0f};
            float pixel_alpha = 0.0f;
            float pixel_depth = 1e6f;
            math::Vector3 pixel_normal = {0.0f, 0.0f, 1.0f};
            math::Vector3 pixel_albedo = {0.0f, 0.0f, 0.0f};
            math::Vector2 pixel_motion = {0.0f, 0.0f};
            std::uint32_t pixel_obj_id = 0;
            std::uint32_t pixel_mat_id = 0;

            for (int s = 0; s < samples_per_pixel_; ++s) {
                // Generate camera ray with jitter for anti-aliasing
                float jitter_x = (samples_per_pixel_ > 1) ? (static_cast<float>(pixel_rng() & 0xFFFF) / 65535.0f - 0.5f) : 0.0f;
                float jitter_y = (samples_per_pixel_ > 1) ? (static_cast<float>(pixel_rng() & 0xFFFF) / 65535.0f - 0.5f) : 0.0f;

                float px = (static_cast<float>(x) + 0.5f + jitter_x) / static_cast<float>(width);
                float py = (static_cast<float>(y) + 0.5f + jitter_y) / static_cast<float>(height);

                // Generate camera ray
                float u = (2.0f * px - 1.0f) * aspect * half_height;
                float v = (1.0f - 2.0f * py) * half_height;
                math::Vector3 direction = (cam_forward + cam_right * u + cam_up * v).normalized();
                math::Vector3 origin = cam.position;

                if (cam.aperture > 0.0f && cam.focal_distance > 0.0f) {
                    const float lens_radius = std::max(0.0f, cam.focal_length_mm / std::max(cam.aperture, 1.0f)) * 0.001f;
                    const math::Vector2 lens = sample_concentric_disk(
                        static_cast<float>(pixel_rng()) / static_cast<float>(pixel_rng.max()),
                        static_cast<float>(pixel_rng()) / static_cast<float>(pixel_rng.max()));
                    const math::Vector3 lens_offset = cam_right * (lens.x * lens_radius) + cam_up * (lens.y * lens_radius);
                    const math::Vector3 focus_point = cam.position + direction * cam.focal_distance;
                    origin = origin + lens_offset;
                    direction = (focus_point - origin).normalized();
                }

                // Motion blur time jitter
                float sample_time = time;
                if (motion_blur && motion_blur->is_enabled()) {
                    const auto& mb_config = motion_blur->get_config();
                    const float shutter_duration = static_cast<float>(frame_duration_seconds * (mb_config.shutter_angle / 360.0));
                    const float shutter_offset = static_cast<float>(frame_duration_seconds * (mb_config.shutter_phase / 360.0));
                    
                    const float jitter_t = (static_cast<float>(pixel_rng() & 0xFFFF) / 65535.0f);
                    sample_time += shutter_offset + jitter_t * shutter_duration;
                }

                ShadingResult sample = trace_ray(origin, direction, scene, pixel_rng, 0, sample_time);
                pixel_color = pixel_color + sample.color;
                pixel_alpha = std::max(pixel_alpha, sample.alpha);
                pixel_depth = std::min(pixel_depth, sample.depth);
                pixel_normal = pixel_normal + sample.normal;
                pixel_albedo = pixel_albedo + sample.albedo;
                pixel_motion = pixel_motion + sample.motion_vector;
                pixel_obj_id = sample.object_id;
                pixel_mat_id = sample.material_id;
            }

            float inv_samples = 1.0f / static_cast<float>(samples_per_pixel_);
            pixel_color = pixel_color * inv_samples;
            pixel_normal = (samples_per_pixel_ > 1) ? (pixel_normal * inv_samples).normalized() : pixel_normal;
            pixel_albedo = pixel_albedo * inv_samples;
            pixel_motion = pixel_motion * inv_samples;

            // Write to AOV buffer vectors directly
            std::size_t idx = static_cast<std::size_t>(y) * width + x;
            out_buffer.beauty_rgba[idx * 4 + 0] = pixel_color.x;
            out_buffer.beauty_rgba[idx * 4 + 1] = pixel_color.y;
            out_buffer.beauty_rgba[idx * 4 + 2] = pixel_color.z;
            out_buffer.beauty_rgba[idx * 4 + 3] = pixel_alpha;
            out_buffer.depth_z[idx] = pixel_depth;
            out_buffer.normal_xyz[idx * 3 + 0] = pixel_normal.x;
            out_buffer.normal_xyz[idx * 3 + 1] = pixel_normal.y;
            out_buffer.normal_xyz[idx * 3 + 2] = pixel_normal.z;
            out_buffer.albedo_rgb[idx * 3 + 0] = pixel_albedo.x;
            out_buffer.albedo_rgb[idx * 3 + 1] = pixel_albedo.y;
            out_buffer.albedo_rgb[idx * 3 + 2] = pixel_albedo.z;
            out_buffer.motion_vector_xy[idx * 2 + 0] = pixel_motion.x;
            out_buffer.motion_vector_xy[idx * 2 + 1] = pixel_motion.y;
            out_buffer.object_id[idx] = pixel_obj_id;
            out_buffer.material_id[idx] = pixel_mat_id;
        }
    }

    if (denoiser_enabled_) {
        denoise_aov_buffer(out_buffer);
    }
}

void RayTracer::denoise_aov_buffer(AOVBuffer& buffer) const {
    if (buffer.width == 0 || buffer.height == 0 || buffer.beauty_rgba.empty()) {
        return;
    }

#ifdef _WIN32
    if (oidn_device_ && oidn_filter_) {
        const std::uint32_t w = buffer.width;
        const std::uint32_t h = buffer.height;

        // We want to avoid copies. OIDN can read from beauty_rgba directly 
        // if we use the correct strides.
        
        oidn_filter_.setImage("color", buffer.beauty_rgba.data(), oidn::Format::Float3, w, h, 0, 4U * sizeof(float), 4U * w * sizeof(float));
        if (!buffer.albedo_rgb.empty()) {
            oidn_filter_.setImage("albedo", buffer.albedo_rgb.data(), oidn::Format::Float3, w, h, 0, 3U * sizeof(float), 3U * w * sizeof(float));
        }
        if (!buffer.normal_xyz.empty()) {
            oidn_filter_.setImage("normal", buffer.normal_xyz.data(), oidn::Format::Float3, w, h, 0, 3U * sizeof(float), 3U * w * sizeof(float));
        }
        
        // Output can be written directly to beauty_rgba but OIDN needs 3 components 
        // while our buffer is 4. So we need a temporary 3-component buffer or 
        // write back.
        // For "Bare Metal" we reuse a static buffer.
        static thread_local std::vector<float> denoised_rgb;
        denoised_rgb.resize(static_cast<std::size_t>(w) * h * 3U);

        oidn_filter_.setImage("output", denoised_rgb.data(), oidn::Format::Float3, w, h, 0, 3U * sizeof(float), 3U * w * sizeof(float));
        oidn_filter_.set("hdr", true);
        oidn_filter_.set("cleanAux", true);
        oidn_filter_.commit();
        oidn_filter_.execute();

        const char* error_message = nullptr;
        const auto error_code = oidn_device_.getError(error_message);
        if (error_code == oidn::Error::None) {
            // Write back only RGB, preserve Alpha
            for (std::size_t i = 0; i < static_cast<std::size_t>(w) * h; ++i) {
                const std::size_t src_idx = i * 3U;
                const std::size_t dst_idx = i * 4U;
                buffer.beauty_rgba[dst_idx + 0] = denoised_rgb[src_idx + 0];
                buffer.beauty_rgba[dst_idx + 1] = denoised_rgb[src_idx + 1];
                buffer.beauty_rgba[dst_idx + 2] = denoised_rgb[src_idx + 2];
            }
            return;
        }
    }
#endif

    // Fallback: High-Performance Cross-Bilateral Filter (A-Trous Style)
    const std::uint32_t w = buffer.width;
    const std::uint32_t h = buffer.height;
    
    static thread_local std::vector<float> scratch_buffer;
    scratch_buffer.resize(buffer.beauty_rgba.size());
    
    const float* src_rgba = buffer.beauty_rgba.data();
    const float* src_depth = buffer.depth_z.data();
    const float* src_normal = buffer.normal_xyz.data();
    float* dst_rgba = scratch_buffer.data();

    const float kDepthSigma = 0.1f;
    // const float kNormalSigma = 0.5f;

    // 3x3 optimized pass
    for (std::uint32_t y = 0; y < h; ++y) {
        for (std::uint32_t x = 0; x < w; ++x) {
            const std::size_t pidx = (static_cast<std::size_t>(y) * w + x);
            const float center_d = src_depth[pidx];
            const float* center_n = &src_normal[pidx * 3];
            const float* center_c = &src_rgba[pidx * 4];

            float sum_w = 1.0f;
            float sum_r = center_c[0];
            float sum_g = center_c[1];
            float sum_b = center_c[2];

            // Simple 4-neighborhood for speed in real-time
            const int offsets_x[] = {-1, 1, 0, 0};
            const int offsets_y[] = {0, 0, -1, 1};

            for (int i = 0; i < 4; ++i) {
                int nx = static_cast<int>(x) + offsets_x[i];
                int ny = static_cast<int>(y) + offsets_y[i];
                if (nx < 0 || nx >= (int)w || ny < 0 || ny >= (int)h) continue;

                const std::size_t nidx = (static_cast<std::size_t>(ny) * w + nx);
                const float* neighbor_c = &src_rgba[nidx * 4];
                const float* neighbor_n = &src_normal[nidx * 3];
                const float neighbor_d = src_depth[nidx];

                float d_w = std::exp(-std::abs(center_d - neighbor_d) / kDepthSigma);
                float n_w = std::max(0.0f, center_n[0]*neighbor_n[0] + center_n[1]*neighbor_n[1] + center_n[2]*neighbor_n[2]);
                n_w = std::pow(n_w, 4.0f); // Sharper normal weight
                
                float w_total = d_w * n_w;
                sum_r += neighbor_c[0] * w_total;
                sum_g += neighbor_c[1] * w_total;
                sum_b += neighbor_c[2] * w_total;
                sum_w += w_total;
            }

            dst_rgba[pidx * 4 + 0] = sum_r / sum_w;
            dst_rgba[pidx * 4 + 1] = sum_g / sum_w;
            dst_rgba[pidx * 4 + 2] = sum_b / sum_w;
            dst_rgba[pidx * 4 + 3] = center_c[3]; // Preserve alpha
        }
    }

    buffer.beauty_rgba = std::move(scratch_buffer);
}

void RayTracer::cleanup_scene() {
    if (scene_) {
        rtcReleaseScene(scene_);
        scene_ = nullptr;
    }
    instances_.clear();
    for (auto& [key, entry] : mesh_cache_) {
        if (entry.scene) {
            rtcReleaseScene(entry.scene);
        }
    }
    mesh_cache_.clear();
    current_env_map_ = nullptr;
}

void RayTracer::log_embree_error(void* userPtr, RTCError code, const char* str) {
    auto* self = static_cast<RayTracer*>(userPtr);
    if (self && str) {
        self->m_last_error = std::string("Embree error ") + std::to_string(static_cast<int>(code)) + ": " + str;
    }
}

} // namespace tachyon::renderer3d
