#include "tachyon/renderer3d/lighting/environment_manager.h"
#include "tachyon/renderer3d/materials/pbr_utils.h"
#include <omp.h>
#include <random>

namespace tachyon::renderer3d {

namespace {
 
constexpr int kBrdfLutSize = 512;
constexpr int kBrdfSampleCount = 1024;
constexpr int kPrefilterLevelCount = 5;
constexpr int kPrefilterSampleCount = 1024;
constexpr float kHalf = 0.5f;
constexpr float kQuarter = 0.25f;
constexpr float kMinDot = 0.0001f;

// Hammersley sequence constants
constexpr int kHammersleyModulo = 127;

// BRDF computation constants
constexpr float kRoughnessDivisor = 2.0f;
constexpr float kFresnelPower = 5.0f;
constexpr int kBrdfChannelCount = 2;

// Color channel constants
constexpr int kRgbChannelCount = 3;

// Mipmap level constants
constexpr int kMipShift = 1;

// Spherical coordinate constants
constexpr float kPhiScale = 2.0f;

// Ray tracing constants
constexpr float kMaxDistance = 1e10f;

// Default sky gradient colors
constexpr math::Vector3 kSkyTopColor{0.2f, 0.5f, 1.0f};
constexpr math::Vector3 kSkyBottomColor{0.8f, 0.9f, 1.0f};
 
} // namespace

std::unique_ptr<media::BRDFLut> EnvironmentManager::brdf_lut_ = nullptr;

EnvironmentManager::EnvironmentManager() = default;
EnvironmentManager::~EnvironmentManager() = default;

void EnvironmentManager::ensure_brdf_lut() {
    if (brdf_lut_) return;
    
    const int size = kBrdfLutSize;
    brdf_lut_ = std::make_unique<media::BRDFLut>();
    brdf_lut_->size = size;
    brdf_lut_->data.resize(size * size * 2);
    
    const int num_samples = kBrdfSampleCount;
    #pragma omp parallel for
    for (int y = 0; y < size; ++y) {
        float n_dot_v = (static_cast<float>(y) + kHalf) / size;
        for (int x = 0; x < size; ++x) {
            float roughness = (static_cast<float>(x) + kHalf) / size;
            
            float a = 0.0f;
            float b = 0.0f;
            
            math::Vector3 v;
            v.x = std::sqrt(1.0f - n_dot_v * n_dot_v);
            v.y = 0.0f;
            v.z = n_dot_v;

            for (int i = 0; i < num_samples; ++i) {
                float u1 = static_cast<float>(i) / num_samples;
                float u2 = static_cast<float>(i % kHammersleyModulo) / static_cast<float>(kHammersleyModulo);
                
                math::Vector3 h = pbr::importance_sample_ggx(u1, u2, {0,0,1}, roughness);
                math::Vector3 l = (h * 2.0f * math::Vector3::dot(v, h) - v).normalized();
                
                float n_dot_l = std::max(l.z, kMinDot);
                float n_dot_h = std::max(h.z, kMinDot);
                float v_dot_h = std::max(math::Vector3::dot(v, h), kMinDot);
                
                if (n_dot_l > 0.0f) {
                    float k = (roughness * roughness) / kRoughnessDivisor;
                    auto g_v = [&](float ndv) { return ndv / (ndv * (1.0f - k) + k); };
                    float g = g_v(n_dot_v) * g_v(n_dot_l);
                    float g_vis = (g * v_dot_h) / (n_dot_h * n_dot_v);
                    float fc = std::pow(1.0f - v_dot_h, kFresnelPower);
                    
                    a += (1.0f - fc) * g_vis;
                    b += fc * g_vis;
                }
            }
            
            brdf_lut_->data[(y * size + x) * kBrdfChannelCount + 0] = a / num_samples;
            brdf_lut_->data[(y * size + x) * kBrdfChannelCount + 1] = b / num_samples;
        }
    }
}

void EnvironmentManager::update_prefiltered_env(const media::HDRTextureData* map) {
    if (!map) {
        prefiltered_env_.reset();
        return;
    }
    
    prefiltered_env_ = std::make_unique<media::PreFilteredEnvMap>();
    const int num_levels = kPrefilterLevelCount;
    const int num_samples = kPrefilterSampleCount;
    
    for (int i = 0; i < num_levels; ++i) {
        float roughness = static_cast<float>(i) / (num_levels - 1);
        int w = std::max(1, map->width >> i);
        int h = std::max(1, map->height >> i);
        
        media::PreFilteredEnvMap::Level level;
        level.width = w;
        level.height = h;
        level.data.resize(w * h * kRgbChannelCount);
        
        #pragma omp parallel for schedule(dynamic)
        for (int y = 0; y < h; ++y) {
            std::mt19937 rng(i * 1000 + y);
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);
            
            for (int x = 0; x < w; ++x) {
                float u = (static_cast<float>(x) + kHalf) / w;
                float v = (static_cast<float>(y) + kHalf) / h;
                
                float phi = (u - kHalf) * kPhiScale * pbr::PI;
                float theta = (kHalf - v) * pbr::PI;
                
                math::Vector3 N;
                N.x = std::cos(theta) * std::cos(phi);
                N.y = std::sin(theta);
                N.z = std::cos(theta) * std::sin(phi);
                N = N.normalized();
                
                math::Vector3 prefiltered_color{0, 0, 0};
                float total_weight = 0.0f;
                
                if (roughness == 0.0f) {
                    prefiltered_color = pbr::sample_equirect(*map, N, 0.0f);
                    total_weight = 1.0f;
                } else {
                    for (int s = 0; s < num_samples; ++s) {
                        float u1 = dist(rng);
                        float u2 = dist(rng);
                        math::Vector3 H = pbr::importance_sample_ggx(u1, u2, N, roughness);
                        math::Vector3 L = (H * 2.0f * math::Vector3::dot(N, H) - N).normalized();
                        
                        float n_dot_l = std::max(math::Vector3::dot(N, L), 0.0f);
                        if (n_dot_l > 0.0f) {
                            prefiltered_color += pbr::sample_equirect(*map, L, 0.0f) * n_dot_l;
                            total_weight += n_dot_l;
                        }
                    }
                }
                
                int d_idx = (y * w + x) * kRgbChannelCount;
                if (total_weight > 0.0f) {
                    math::Vector3 final_c = prefiltered_color / total_weight;
                    level.data[d_idx+0] = final_c.x;
                    level.data[d_idx+1] = final_c.y;
                    level.data[d_idx+2] = final_c.z;
                } else {
                    level.data[d_idx+0] = 0;
                    level.data[d_idx+1] = 0;
                    level.data[d_idx+2] = 0;
                }
            }
        }
        prefiltered_env_->levels.push_back(std::move(level));
    }
}

EnvironmentSample EnvironmentManager::sample_environment(const math::Vector3& direction, const media::HDRTextureData* environment_map, float intensity, float rotation) {
    auto res = sample_environment(direction, environment_map, intensity, rotation, 0.0f);
    res.albedo = {0,0,0};
    res.normal = -direction;
    return res;
}

EnvironmentSample EnvironmentManager::sample_environment(const math::Vector3& direction, const media::HDRTextureData* environment_map, float intensity, float rotation, float roughness) {
    if (environment_map) {
        if (prefiltered_env_ && !prefiltered_env_->levels.empty()) {
            float level = roughness * (prefiltered_env_->levels.size() - 1);
            int idx0 = static_cast<int>(std::floor(level));
            int idx1 = std::min(idx0 + 1, static_cast<int>(prefiltered_env_->levels.size() - 1));
            float f = level - idx0;
            
            const auto& l0 = prefiltered_env_->levels[idx0];
            const auto& l1 = prefiltered_env_->levels[idx1];
            
            auto s0 = pbr::sample_equirect({l0.data, l0.width, l0.height, kRgbChannelCount}, direction, rotation);
            auto s1 = pbr::sample_equirect({l1.data, l1.width, l1.height, kRgbChannelCount}, direction, rotation);
            
            return { (s0 * (1.0f - f) + s1 * f) * intensity, 0.0f, kMaxDistance, {0,0,0}, -direction };
        }
        return { pbr::sample_equirect(*environment_map, direction, rotation) * intensity, 0.0f, kMaxDistance, {0,0,0}, -direction };
    }

    float t = kHalf * (direction.y + 1.0f);
    return {kSkyBottomColor * (1.0f - t) + kSkyTopColor * t, 0.0f, kMaxDistance, {0,0,0}, -direction};
}

} // namespace tachyon::renderer3d
