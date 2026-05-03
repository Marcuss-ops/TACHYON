#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

namespace tachyon::profiling {

enum class ProfileEventType {
    Phase,
    Frame,
    Layer,
    Effect,
    Text,
    Asset,
    Cache,
    Encode,
    PipeWrite,
    AudioMux,
    Memory,
    Thread
};

struct ProfileEvent {
    ProfileEventType type;
    std::string name;
    std::string category;

    std::int64_t frame_number{-1};
    std::string layer_id;
    std::string effect_id;
    std::string asset_id;

    std::uint64_t thread_id{0};

    double start_ms{0.0};
    double duration_ms{0.0};

    std::unordered_map<std::string, std::string> tags;
};

} // namespace tachyon::profiling
