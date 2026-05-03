#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace tachyon::profiling {

struct FrameProfile {
    std::int64_t frame_number{0};
    double total_ms{0.0};
    double render_ms{0.0};
    double encode_write_ms{0.0};
    std::size_t cache_hits{0};
    std::size_t cache_misses{0};
};

struct LayerProfile {
    std::string layer_id;
    double total_ms{0.0};
    std::int64_t calls{0};
};

struct EffectProfile {
    std::string effect_id;
    double total_ms{0.0};
    std::int64_t calls{0};
};

struct ThreadProfile {
    std::uint64_t thread_id{0};
    double busy_ms{0.0};
    double idle_ms{0.0};
    std::int64_t frames_rendered{0};
};

struct ProfileSummary {
    double total_ms{0.0};
    double avg_frame_ms{0.0};
    double p95_frame_ms{0.0};
    std::int64_t slowest_frame{-1};
    double slowest_frame_ms{0.0};
    
    std::vector<FrameProfile> frames;
    std::vector<LayerProfile> layers;
    std::vector<EffectProfile> effects;
    std::vector<ThreadProfile> threads;
};

} // namespace tachyon::profiling
