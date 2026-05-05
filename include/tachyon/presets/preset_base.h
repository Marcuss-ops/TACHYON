#pragma once

#include <optional>
#include <string>

namespace tachyon::presets {

struct LayerParams {
    double in_point{0.0};
    double out_point{5.0};
    float  x{0.0f};
    float  y{0.0f};
    float  w{1920.0f};
    float  h{1080.0f};
    float  opacity{1.0f};

    std::string enter_preset;
    double      enter_duration{0.3};
    std::string exit_preset;
    double      exit_duration{0.3};

    [[nodiscard]] double duration() const { return out_point - in_point; }
};

struct SceneParams {
    int    width{1920};
    int    height{1080};
    int    fps{30};
    double duration{5.0};
    std::string name;
    std::string bg_color{"#0d1117"};
    bool   bg_procedural{false};
    std::string bg_kind{};
};

} // namespace tachyon::presets
