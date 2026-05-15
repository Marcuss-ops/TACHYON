#pragma once

#include "tachyon/core/types/media_error.h"
#include <string>
#include <vector>
#include <filesystem>

namespace tachyon::core {

/**
 * @brief Configuration for an individual audio input stream.
 */
struct AudioInput {
    std::string id;
    std::filesystem::path path;
    double volume{1.0};
    double offset_seconds{0.0};
    bool gate_enabled{false};

    AudioInput(std::string id, std::filesystem::path path)
        : id(std::move(id)), path(std::move(path)) {}
};

/**
 * @brief Synchronization configuration for audio.
 */
struct AudioSyncConfig {
    double frame_duration_ms{40.0}; // 25 fps default
};

/**
 * @brief Declarative audio graph configuration.
 * Derived from ruststream-core/core/audio_graph.rs
 */
struct AudioGraphConfig {
    std::string graph_id;
    std::vector<AudioInput> inputs;
    int output_sample_rate{48000};
    int output_channels{2};
    long total_duration_samples{0};
    AudioSyncConfig sync;

    explicit AudioGraphConfig(std::string id) : graph_id(std::move(id)) {}

    void add_input(AudioInput input) {
        inputs.push_back(std::move(input));
    }

    [[nodiscard]] bool validate() const {
        if (graph_id.empty() || inputs.empty()) return false;
        if (output_sample_rate <= 0 || (output_channels != 1 && output_channels != 2)) return false;
        for (const auto& input : inputs) {
            if (input.path.empty()) return false;
        }
        return true;
    }
};

} // namespace tachyon::core
