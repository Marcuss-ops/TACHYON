#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace tachyon {

struct AnimationPreset {
    std::string name;
    std::string phase;  // "in", "during", "out"
    struct Keyframe {
        float t;  // 0.0 - 1.0 normalized
        std::optional<float> scale_x, scale_y;
        std::optional<float> position_x, position_y;
        std::optional<float> opacity;
        std::optional<float> rotation;
        std::optional<float> blur_radius;
    };
    std::vector<Keyframe> keyframes;
};

class PresetLibrary {
public:
    bool load_from_directory(const std::filesystem::path& dir);
    const AnimationPreset* find(const std::string& name) const;
private:
    std::unordered_map<std::string, AnimationPreset> m_presets;
};

class PresetCompiler {
public:
    explicit PresetCompiler(const PresetLibrary& library);

    // Expands all presets into concrete keyframes.
    // Modifies the SceneSpec in-place before rendering.
    void expand(SceneSpec& scene) const;

private:
    void expand_layer(LayerSpec& layer) const;
    void inject_phase(LayerSpec& layer,
                      const AnimationPreset& preset,
                      double phase_start,
                      double phase_end) const;

    const PresetLibrary& m_library;
};

} // namespace tachyon
