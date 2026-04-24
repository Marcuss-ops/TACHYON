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
    const MotionBlurRenderer* /*motion_blur*/,
    double frame_time_seconds,
    double frame_duration_seconds) {
    (void)frame_duration_seconds;

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

    std::random_device rd;
    std::mt19937 rng(rd());

    const float time = static_cast<float>(frame_time_seconds);

    // Compute camera vectors
    math::Vector3 cam_forward = (cam.target - cam.position).normalized();
    math::Vector3 cam_right = math::Vector3::cross(cam_forward, cam.up).normalized();
    math::Vector3 cam_up = math::Vector3::cross(cam_right, cam_forward).normalized();
    float fov_rad = cam.fov_y * 3.1415926535f / 180.0f;
    float half_height = std::tan(fov_rad * 0.5f);
    float aspect = static_cast<float>(width) / static_cast<float>(height);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
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
                float jitter_x = (samples_per_pixel_ > 1) ? (static_cast<float>(rng() & 0xFFFF) / 65535.0f - 0.5f) : 0.0f;
                float jitter_y = (samples_per_pixel_ > 1) ? (static_cast<float>(rng() & 0xFFFF) / 65535.0f - 0.5f) : 0.0f;

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
                        static_cast<float>(rng()) / static_cast<float>(rng.max()),
                        static_cast<float>(rng()) / static_cast<float>(rng.max()));
                    const math::Vector3 lens_offset = cam_right * (lens.x * lens_radius) + cam_up * (lens.y * lens_radius);
                    const math::Vector3 focus_point = cam.position + direction * cam.focal_distance;
                    origin = origin + lens_offset;
                    direction = (focus_point - origin).normalized();
                }

                ShadingResult sample = trace_ray(origin, direction, scene, rng, 0, time);
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

    const std::vector<float> src = buffer.beauty_rgba;
    const std::vector<float> src_depth = buffer.depth_z;
    const std::vector<float> src_normal = buffer.normal_xyz;
    const std::uint32_t w = buffer.width;
    const std::uint32_t h = buffer.height;

    auto normal_at = [&](std::uint32_t x, std::uint32_t y) -> math::Vector3 {
        const std::size_t idx = (static_cast<std::size_t>(y) * w + x) * 3;
        return {src_normal[idx + 0], src_normal[idx + 1], src_normal[idx + 2]};
    };

    auto color_at = [&](std::uint32_t x, std::uint32_t y) -> math::Vector3 {
        const std::size_t idx = (static_cast<std::size_t>(y) * w + x) * 4;
        return {src[idx + 0], src[idx + 1], src[idx + 2]};
    };

    std::vector<float> dst = src;
    const float kDepthSigma = 0.08f;
    const float kNormalSigma = 0.35f;
    const float kColorSigma = 0.25f;

    for (std::uint32_t y = 0; y < h; ++y) {
        for (std::uint32_t x = 0; x < w; ++x) {
            const std::size_t pidx = static_cast<std::size_t>(y) * w + x;
            const float depth = src_depth[pidx];
            const math::Vector3 normal = normal_at(x, y);
            const math::Vector3 center = color_at(x, y);

            math::Vector3 accum{0.0f, 0.0f, 0.0f};
            float wsum = 0.0f;

            for (int oy = -1; oy <= 1; ++oy) {
                const int ny = std::clamp(static_cast<int>(y) + oy, 0, static_cast<int>(h) - 1);
                for (int ox = -1; ox <= 1; ++ox) {
                    const int nx = std::clamp(static_cast<int>(x) + ox, 0, static_cast<int>(w) - 1);
                    const std::size_t nidx = static_cast<std::size_t>(ny) * w + static_cast<std::size_t>(nx);
                    const float ndepth = src_depth[nidx];
                    const math::Vector3 nnormal = normal_at(static_cast<std::uint32_t>(nx), static_cast<std::uint32_t>(ny));
                    const math::Vector3 ncolor = color_at(static_cast<std::uint32_t>(nx), static_cast<std::uint32_t>(ny));

                    const float depth_weight = std::exp(-std::abs(ndepth - depth) / kDepthSigma);
                    const float normal_weight = std::exp(-std::max(0.0f, 1.0f - math::Vector3::dot(normal, nnormal)) / kNormalSigma);
                    const float color_dist = (ncolor - center).length();
                    const float color_weight = std::exp(-color_dist / kColorSigma);
                    const float weight = depth_weight * normal_weight * color_weight;

                    accum += ncolor * weight;
                    wsum += weight;
                }
            }

            if (wsum > 1.0e-6f) {
                accum /= wsum;
                const std::size_t out_idx = pidx * 4;
                dst[out_idx + 0] = accum.x;
                dst[out_idx + 1] = accum.y;
                dst[out_idx + 2] = accum.z;
            }
        }
    }

    buffer.beauty_rgba = std::move(dst);
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
