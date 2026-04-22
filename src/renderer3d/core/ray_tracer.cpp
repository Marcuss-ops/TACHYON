#ifdef _MSC_VER
#pragma warning(disable: 4324)
#endif

#include "tachyon/renderer3d/core/ray_tracer.h"
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

// Stub implementation for compilation completeness. A full PBR shader would be called here.
ShadingResult RayTracer::trace_ray(const math::Vector3& origin, const math::Vector3& direction, const EvaluatedScene3D& scene, std::mt19937& rng, int depth) {
    ShadingResult res;
    res.color = {0,0,0}; res.alpha = 0; res.depth = 1e5f;
    res.normal = {0,0,1}; res.albedo = {0,0,0};
    res.object_id = 0; res.material_id = 0;

    RTCIntersectContext context;
    rtcInitIntersectContext(&context);
    RTCRayHit rayhit;
    rayhit.ray.org_x = origin.x; rayhit.ray.org_y = origin.y; rayhit.ray.org_z = origin.z;
    rayhit.ray.dir_x = direction.x; rayhit.ray.dir_y = direction.y; rayhit.ray.dir_z = direction.z;
    rayhit.ray.tnear = 0.001f; rayhit.ray.tfar = 1e5f;
    rayhit.ray.mask = -1; rayhit.ray.flags = 0;
    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

    rtcIntersect1(scene_, &context, &rayhit);

    if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
        // Find instance
        for (const auto& inst : instances_) {
            if (inst.geom_id == rayhit.hit.instID[0]) {
                res.object_id = inst.object_id;
                res.material_id = inst.material_id;
                res.depth = rayhit.ray.tfar;
                res.alpha = 1.0f;
                
                // Normal
                math::Vector3 n{rayhit.hit.Ng_x, rayhit.hit.Ng_y, rayhit.hit.Ng_z};
                res.normal = n.normalized();
                
                // Albedo
                res.albedo = {
                    static_cast<float>(inst.material.base_color.r) / 255.0f,
                    static_cast<float>(inst.material.base_color.g) / 255.0f,
                    static_cast<float>(inst.material.base_color.b) / 255.0f
                };
                
                // Simple lighting
                float ndotl = std::max(0.0f, -math::Vector3::dot(direction, res.normal));
                res.color = res.albedo * ndotl;
                break;
            }
        }
    }
    return res;
}

void RayTracer::render(
    const EvaluatedScene3D& scene,
    AOVBuffer& out_buffer,
    const MotionBlurRenderer* motion_blur,
    double frame_time_seconds,
    double frame_duration_seconds) {
    
    if (!scene_ || instances_.empty() || out_buffer.width == 0 || out_buffer.height == 0) return;
    
    const auto& eval_cam = scene.camera;

    math::Vector3 cam_pos = eval_cam.position;
    
    // Compute basis vectors from target. Simple look-at math.
    math::Vector3 forward = (eval_cam.target - eval_cam.position).normalized();
    if (forward.length_squared() < 1e-6f) forward = {0, 0, -1};
    math::Vector3 right = math::Vector3::cross(forward, eval_cam.up).normalized();
    math::Vector3 up = math::Vector3::cross(right, forward).normalized();

    float aspect = static_cast<float>(out_buffer.width) / static_cast<float>(out_buffer.height);
    float tan_half_fov = std::tan(eval_cam.fov_y * 3.14159265f / 180.0f * 0.5f);

    std::vector<double> blur_times;
    std::vector<float> blur_weights;
    if (motion_blur && motion_blur->is_enabled()) {
        blur_times = motion_blur->compute_subframe_times(frame_time_seconds, frame_duration_seconds);
        blur_weights.reserve(blur_times.size());
        for (std::size_t i = 0; i < blur_times.size(); ++i) {
            blur_weights.push_back(motion_blur->evaluate_weight(static_cast<int>(i), static_cast<int>(blur_times.size())));
        }
    }
    if (blur_times.empty()) {
        blur_times.push_back(frame_time_seconds);
        blur_weights.push_back(1.0f);
    }

    float blur_weight_sum = 0.0f;
    for (float weight : blur_weights) blur_weight_sum += weight;
    blur_weight_sum = std::max(blur_weight_sum, 1e-6f);

    #pragma omp parallel for schedule(dynamic)
    for (int y = 0; y < static_cast<int>(out_buffer.height); ++y) {
        for (int x = 0; x < static_cast<int>(out_buffer.width); ++x) {
            math::Vector3 acc_rgb{0,0,0}, f_alb{0,0,0}, f_nor{0,0,1};
            math::Vector2 f_mv{0,0};
            float acc_alpha = 0.0f, acc_depth = 0.0f;
            std::uint32_t first_obj = 0, first_mat = 0;
            int spp = std::max(1, samples_per_pixel_);

            for (std::size_t bt = 0; bt < blur_times.size(); ++bt) {
                const double sample_time = blur_times[bt];
                std::mt19937 rng(static_cast<unsigned int>(y * out_buffer.width + x + static_cast<int>(sample_time * 1000.0)));
                std::uniform_real_distribution<float> dist(0.0f, 1.0f);
                const float motion_weight = blur_weights[bt];

                for (int s = 0; s < spp; ++s) {
                    float u = (x + (spp > 1 ? dist(rng) : 0.5f)) / out_buffer.width;
                    float v = (y + (spp > 1 ? dist(rng) : 0.5f)) / out_buffer.height;
                    
                    math::Vector3 rd = (forward + right * (2*u-1)*aspect*tan_half_fov + up * (1-2*v)*tan_half_fov).normalized();
                    math::Vector3 origin = cam_pos;
                    
                    if (eval_cam.aperture > 0.0f) {
                        math::Vector3 fp = cam_pos + rd * eval_cam.focal_distance; 
                        
                        // Disk sampling
                        float r = std::sqrt(dist(rng)) * eval_cam.aperture;
                        float theta = dist(rng) * 2.0f * 3.14159265f;
                        math::Vector2 ls = {r * std::cos(theta), r * std::sin(theta)};
                        
                        origin += right * ls.x + up * ls.y; 
                        rd = (fp - origin).normalized();
                    }
                    
                    auto result = trace_ray(origin, rd, scene, rng, 0);
                    acc_rgb += result.color * motion_weight;
                    acc_alpha += result.alpha * motion_weight;
                    acc_depth += result.depth * motion_weight;
                    
                    if (s == 0 && bt == 0) { 
                        f_alb = result.albedo; 
                        f_nor = result.normal; 
                        first_obj = result.object_id;
                        first_mat = result.material_id;
                        
                        // Calculate Motion Vector
                        if (result.depth < 1e4f && result.object_id != 0) {
                            math::Vector3 hit_world = origin + rd * result.depth;
                            const GeoInstance* hit_inst = nullptr;
                            for (const auto& inst : instances_) {
                                if (inst.object_id == first_obj) { hit_inst = &inst; break; }
                            }
                            
                            if (hit_inst && hit_inst->previous_world_transform) {
                                math::Matrix4x4 inv_world = math::inverse_affine(hit_inst->world_transform);
                                math::Vector3 hit_local = math::transform_point(inv_world, hit_world);
                                math::Vector3 prev_hit_world = math::transform_point(*hit_inst->previous_world_transform, hit_local);
                                
                                math::Vector3 cam_pos_curr = scene.camera.position;
                                math::Vector3 fwd_curr = (scene.camera.target - scene.camera.position).normalized();
                                if (fwd_curr.length_squared() < 1e-6f) fwd_curr = {0,0,-1};
                                math::Vector3 right_curr = math::Vector3::cross(fwd_curr, scene.camera.up).normalized();
                                math::Vector3 up_curr = math::Vector3::cross(right_curr, fwd_curr).normalized();
                                float z_curr = math::Vector3::dot(hit_world - cam_pos_curr, fwd_curr);
                                
                                if (z_curr > 1e-4f) {
                                    float x_curr = math::Vector3::dot(hit_world - cam_pos_curr, right_curr) / (z_curr * aspect * tan_half_fov);
                                    float y_curr = math::Vector3::dot(hit_world - cam_pos_curr, up_curr) / (z_curr * tan_half_fov);
                                    float u_curr = (x_curr + 1.0f) * 0.5f;
                                    float v_curr = (1.0f - y_curr) * 0.5f;
                                    
                                    math::Vector3 cam_pos_prev = scene.camera.previous_position.value_or(scene.camera.position);
                                    math::Vector3 target_prev = scene.camera.previous_target.value_or(scene.camera.target);
                                    math::Vector3 up_v_prev = scene.camera.previous_up.value_or(scene.camera.up);
                                    
                                    math::Vector3 fwd_prev = (target_prev - cam_pos_prev).normalized();
                                    if(fwd_prev.length_squared() < 1e-6f) fwd_prev = {0,0,-1};
                                    math::Vector3 right_prev = math::Vector3::cross(fwd_prev, up_v_prev).normalized();
                                    math::Vector3 up_prev = math::Vector3::cross(right_prev, fwd_prev).normalized();
                                    
                                    float z_prev = math::Vector3::dot(prev_hit_world - cam_pos_prev, fwd_prev);
                                    if (z_prev > 1e-4f) {
                                        float x_prev = math::Vector3::dot(prev_hit_world - cam_pos_prev, right_prev) / (z_prev * aspect * tan_half_fov);
                                        float y_prev = math::Vector3::dot(prev_hit_world - cam_pos_prev, up_prev) / (z_prev * tan_half_fov);
                                        float u_prev = (x_prev + 1.0f) * 0.5f;
                                        float v_prev = (1.0f - y_prev) * 0.5f;
                                        
                                        f_mv = { (u_curr - u_prev) * out_buffer.width, (v_curr - v_prev) * out_buffer.height };
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            std::size_t idx = static_cast<std::size_t>(y) * out_buffer.width + x;
            const float denom = static_cast<float>(spp) * blur_weight_sum;
            
            // Beauty Pass
            out_buffer.beauty_rgba[idx*4+0] = acc_rgb.x / denom; 
            out_buffer.beauty_rgba[idx*4+1] = acc_rgb.y / denom; 
            out_buffer.beauty_rgba[idx*4+2] = acc_rgb.z / denom; 
            out_buffer.beauty_rgba[idx*4+3] = std::clamp(acc_alpha / denom, 0.0f, 1.0f);
            
            // Z Depth
            out_buffer.depth_z[idx] = acc_depth / denom;
            
            // Normals
            out_buffer.normal_xyz[idx*3+0] = f_nor.x;
            out_buffer.normal_xyz[idx*3+1] = f_nor.y;
            out_buffer.normal_xyz[idx*3+2] = f_nor.z;
            
            // IDs and MV
            out_buffer.object_id[idx] = first_obj;
            out_buffer.material_id[idx] = first_mat;
            out_buffer.motion_vector_xy[idx] = f_mv;
        }
    }

    // Denoise pass
    if (oidn_filter_ && samples_per_pixel_ <= 4) {
        oidn_filter_.setImage("color",  out_buffer.beauty_rgba.data(), oidn::Format::Float3, (size_t)out_buffer.width, (size_t)out_buffer.height, 0, 4 * sizeof(float));
        oidn_filter_.setImage("output", out_buffer.beauty_rgba.data(), oidn::Format::Float3, (size_t)out_buffer.width, (size_t)out_buffer.height, 0, 4 * sizeof(float));
        oidn_filter_.commit(); 
        oidn_filter_.execute();
    }
}

} // namespace tachyon::renderer3d
