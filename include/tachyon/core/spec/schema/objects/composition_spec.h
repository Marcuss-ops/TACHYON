#pragma once

#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/objects/camera2d_spec.h"
#include "tachyon/core/spec/schema/audio/audio_spec.h"
#include "tachyon/camera_cut_contract.h"
#include "tachyon/core/spec/schema/contracts/shared_contracts.h"
#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <unordered_map>
#include <map>
#include <nlohmann/json.hpp>

namespace tachyon {

struct FrameRate {
    std::int64_t numerator{60};
    std::int64_t denominator{1};

    [[nodiscard]] double value() const noexcept {
        if (denominator == 0) {
            return 0.0;
        }
        return static_cast<double>(numerator) / static_cast<double>(denominator);
    }
};

struct CompositionSpec {
    std::string id;
    std::string name;
    int width{1920};
    int height{1080};
    double duration{10.0};
    FrameRate frame_rate;
    std::optional<std::int64_t> fps;
    std::optional<std::string> background;
    std::optional<std::string> environment_path;
    std::vector<LayerSpec> layers;
    std::vector<AudioTrackSpec> audio_tracks;
    std::vector<CameraCut> camera_cuts;
    std::vector<Camera2DSpec> cameras_2d;
    std::optional<std::string> active_camera2d_id;

    /// Remotion-like input props: parameterized data for this composition.
    /// Can be accessed in expressions via prop("key") or in layer properties.
    std::map<std::string, nlohmann::json> input_props;
};

} // namespace tachyon
