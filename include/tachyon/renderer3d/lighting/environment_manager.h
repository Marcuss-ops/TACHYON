#pragma once

#include "tachyon/core/math/vector3.h"
#include "tachyon/media/loading/mesh_asset.h"
#include <memory>
#include <vector>

namespace tachyon::renderer3d {

struct ShadingResult {
    math::Vector3 color;
    float alpha;
    float depth;
    math::Vector3 albedo;
    math::Vector3 normal;
};

class EnvironmentManager {
public:
    EnvironmentManager();
    ~EnvironmentManager();

    void update_prefiltered_env(const media::HDRTextureData* map);
    void ensure_brdf_lut();

    ShadingResult sample_environment(const math::Vector3& direction, const media::HDRTextureData* environment_map, float intensity, float rotation);
    ShadingResult sample_environment(const math::Vector3& direction, const media::HDRTextureData* environment_map, float intensity, float rotation, float roughness);

    const media::PreFilteredEnvMap* get_prefiltered_env() const { return prefiltered_env_.get(); }
    static const media::BRDFLut* get_brdf_lut() { return brdf_lut_.get(); }

private:
    std::unique_ptr<media::PreFilteredEnvMap> prefiltered_env_{nullptr};
    static std::unique_ptr<media::BRDFLut> brdf_lut_;
};

} // namespace tachyon::renderer3d
