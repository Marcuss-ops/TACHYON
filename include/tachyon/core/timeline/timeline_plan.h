#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <algorithm>

namespace tachyon::core {

/**
 * @brief Represents a timebase for media calculations.
 * Derived from ruststream-core/core/timeline.rs
 */
struct Timebase {
    std::uint32_t numerator{1};
    std::uint32_t denominator{1};

    [[nodiscard]] double to_seconds(std::int64_t timestamp) const noexcept {
        if (denominator == 0) return 0.0;
        return static_cast<double>(timestamp) * numerator / denominator;
    }

    [[nodiscard]] std::int64_t from_seconds(double seconds) const noexcept {
        if (numerator == 0) return 0;
        return static_cast<std::int64_t>(seconds * denominator / numerator);
    }

    [[nodiscard]] bool validate() const noexcept {
        return numerator > 0 && denominator > 0;
    }
};

/**
 * @brief A segment of video within a track.
 */
struct VideoSegment {
    std::string path;
    std::int64_t start{0};           // Start in source (timestamps)
    std::int64_t duration{0};        // Duration (timestamps)
    std::int64_t timeline_start{0};  // Start on timeline (timestamps)

    VideoSegment(std::string path, std::int64_t start, std::int64_t duration, std::int64_t timeline_start)
        : path(std::move(path)), start(start), duration(duration), timeline_start(timeline_start) {}
};

/**
 * @brief A track containing multiple video segments.
 */
struct VideoTrack {
    std::string id;
    bool is_primary{false};
    std::vector<VideoSegment> segments;

    explicit VideoTrack(std::string id) : id(std::move(id)) {}

    void add_segment(VideoSegment segment) {
        segments.push_back(std::move(segment));
    }

    [[nodiscard]] bool validate() const noexcept {
        return !id.empty() && !segments.empty();
    }
};

/**
 * @brief An effect node applied to a time range.
 */
struct EffectNode {
    std::string effect_type;
    std::int64_t start_time{0};
    std::int64_t duration{0};
    std::vector<std::pair<std::string, std::string>> parameters;

    EffectNode(std::string type, std::int64_t start, std::int64_t duration)
        : effect_type(std::move(type)), start_time(start), duration(duration) {}
};

/**
 * @brief An overlay track for graphics or secondary video.
 */
struct OverlayTrack {
    std::string id;
    std::string path;
    std::int64_t start_time{0};
    std::int64_t duration{0};
    double x{0.0};
    double y{0.0};
    double opacity{1.0};

    OverlayTrack(std::string id, std::string path)
        : id(std::move(id)), path(std::move(path)) {}

    [[nodiscard]] bool validate() const noexcept {
        return !id.empty() && !path.empty() && duration > 0 && start_time >= 0;
    }
};

/**
 * @brief Configuration for the final output.
 */
struct OutputConfig {
    std::string path;
    std::uint32_t width{1920};
    std::uint32_t height{1080};
    double fps{30.0};
    std::uint32_t crf{23};
    std::string video_codec{"libx264"};
    std::string audio_codec{"aac"};
    std::uint32_t audio_bitrate_kbps{256};
    std::uint32_t sample_rate{48000};
};

/**
 * @brief Complete declarative plan for a media render job.
 */
class MediaTimelinePlan {
public:
    std::string plan_id;
    Timebase timebase{1, 1000}; // Default to ms
    OutputConfig output;
    std::vector<VideoTrack> video_tracks;
    std::vector<EffectNode> effects;
    std::vector<OverlayTrack> overlays;
    
    std::uint32_t threads{0}; // 0 = auto
    bool use_simd{true};
    bool emergency_fallback_enabled{false};
    bool allow_intermediate_files{true};

    explicit MediaTimelinePlan(std::string id) : plan_id(std::move(id)) {}

    void add_track(VideoTrack track) {
        video_tracks.push_back(std::move(track));
    }

    void add_effect(EffectNode effect) {
        effects.push_back(std::move(effect));
    }

    void add_overlay(OverlayTrack overlay) {
        overlays.push_back(std::move(overlay));
    }

    [[nodiscard]] const VideoTrack* find_primary_track() const noexcept {
        for (const auto& track : video_tracks) {
            if (track.is_primary) return &track;
        }
        return video_tracks.empty() ? nullptr : &video_tracks[0];
    }

    [[nodiscard]] bool validate() const noexcept {
        if (plan_id.empty()) return false;
        if (!timebase.validate()) return false;
        if (output.path.empty()) return false;
        if (video_tracks.empty()) return false;
        
        for (const auto& track : video_tracks) {
            if (!track.validate()) return false;
        }
        
        for (const auto& overlay : overlays) {
            if (!overlay.validate()) return false;
        }
        
        return true;
    }
};

} // namespace tachyon::core
