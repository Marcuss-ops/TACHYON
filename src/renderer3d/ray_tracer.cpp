#ifdef _MSC_VER
#pragma warning(disable: 4324)
#endif

#include "tachyon/renderer3d/ray_tracer.h"
#include "tachyon/media/extruder.h"
#include "tachyon/renderer3d/pbr_utils.h"
#include <iostream>
#include <random>
#include <cmath>
#include <algorithm>

namespace tachyon::renderer3d {

void RayTracer::log_embree_error(void* userPtr, RTCError code, const char* str) {
    (void)userPtr;
    if (code == RTC_ERROR_NONE) return;
    std::cerr << "Embree Error [" << code << "]: " << str << std::endl;
}

RayTracer::RayTracer() {
    device_ = rtcNewDevice(nullptr);
    if (!device_) return;
    rtcSetDeviceErrorFunction(device_, log_embree_error, nullptr);
    oidn_device_ = oidn::newDevice();
    oidn_device_.commit();
    oidn_filter_ = oidn_device_.newFilter("RT");
}

RayTracer::~RayTracer() {
    cleanup_scene();
    for (auto& entry : mesh_cache_) rtcReleaseScene(entry.second.scene);
    mesh_cache_.clear();
    if (device_) rtcReleaseDevice(device_);
}

void RayTracer::render(const scene::EvaluatedCompositionState& state, float* out_rgba, float* out_depth, int width, int height) {
    if (!scene_ || instances_.empty()) return;
    const auto& eval_cam = state.camera;
    if (!eval_cam.available) return;

    math::Vector3 cam_pos = eval_cam.position;
    math::Vector3 forward = eval_cam.camera.transform.rotation.rotate_vector({0, 0, -1}).normalized();
    math::Vector3 right   = eval_cam.camera.transform.rotation.rotate_vector({1, 0, 0}).normalized();
    math::Vector3 up      = eval_cam.camera.transform.rotation.rotate_vector({0, 1, 0}).normalized();

    float tan_half_fov = std::tan(eval_cam.camera.fov_y_rad * 0.5f);
    float aspect = eval_cam.camera.aspect;

    std::vector<float> albedo_aux;
    std::vector<float> normal_aux;
    if (oidn_filter_ && samples_per_pixel_ <= 4) {
        albedo_aux.resize(width * height * 3);
        normal_aux.resize(width * height * 3);
    }

    #pragma omp parallel for schedule(dynamic)
    for (int y = 0; y < height; ++y) {
        std::mt19937 rng(static_cast<unsigned int>(state.frame_number + y * width));
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        for (int x = 0; x < width; ++x) {
            math::Vector3 acc_rgb{0,0,0}, f_alb{0,0,0}, f_nor{0,0,1};
            float acc_alpha = 0.0f, acc_depth = 0.0f;
            int spp = std::max(1, samples_per_pixel_);

            for (int s = 0; s < spp; ++s) {
                float u = (x + (spp > 1 ? dist(rng) : 0.5f)) / width, v = (y + (spp > 1 ? dist(rng) : 0.5f)) / height;
                math::Vector3 rd = (forward + right * (2*u-1)*aspect*tan_half_fov + up * (1-2*v)*tan_half_fov).normalized(), origin = cam_pos;
                if (eval_cam.dof_enabled && eval_cam.aperture > 0.0f) {
                    math::Vector3 fp = cam_pos + rd * eval_cam.focus_distance; math::Vector2 ls = pbr::sample_disk(eval_cam.aperture, dist(rng), dist(rng));
                    origin += right * ls.x + up * ls.y; rd = (fp - origin).normalized();
                }
                auto result = trace_ray(origin, rd, state, rng, 0);
                acc_rgb += result.color; acc_alpha += result.alpha; acc_depth += result.depth;
                if (s == 0) { f_alb = result.albedo; f_nor = result.normal; }
            }
            int idx = (y * width + x) * 4;
            out_rgba[idx+0] = acc_rgb.x / spp; out_rgba[idx+1] = acc_rgb.y / spp; out_rgba[idx+2] = acc_rgb.z / spp; out_rgba[idx+3] = std::clamp(acc_alpha / spp, 0.0f, 1.0f);
            if (out_depth) out_depth[y * width + x] = acc_depth / spp;
            if (!albedo_aux.empty()) {
                int aidx = (y * width + x) * 3;
                albedo_aux[aidx+0] = f_alb.x; albedo_aux[aidx+1] = f_alb.y; albedo_aux[aidx+2] = f_alb.z;
                normal_aux[aidx+0] = f_nor.x; normal_aux[aidx+1] = f_nor.y; normal_aux[aidx+2] = f_nor.z;
            }
        }
    }

    if (oidn_filter_ && samples_per_pixel_ <= 4) {
        oidn_filter_.setImage("color",  out_rgba, oidn::Format::Float3, (size_t)width, (size_t)height, 0, 4 * sizeof(float));
        oidn_filter_.setImage("albedo", albedo_aux.data(), oidn::Format::Float3, (size_t)width, (size_t)height);
        oidn_filter_.setImage("normal", normal_aux.data(), oidn::Format::Float3, (size_t)width, (size_t)height);
        oidn_filter_.setImage("output", out_rgba, oidn::Format::Float3, (size_t)width, (size_t)height, 0, 4 * sizeof(float));
        oidn_filter_.commit(); oidn_filter_.execute();
    }
}

} // namespace tachyon::renderer3d
