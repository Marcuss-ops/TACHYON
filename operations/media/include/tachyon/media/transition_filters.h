#pragma once

#include <string>
#include <vector>

namespace tachyon::media {

/**
 * @brief Builders for FFmpeg transition filters (fade, wipe, circle, etc.)
 */
class TransitionFilters {
public:
    enum class Type {
        Fade,
        WipeLeft,
        WipeRight,
        CircleCrop
    };

    struct Params {
        Type type = Type::Fade;
        double start_time = 0.0;
        double duration = 0.5;
        bool is_out = false; // true if fading/wiping OUT
    };

    /**
     * @brief Builds a filter_complex string for a single transition.
     * Note: This usually requires input labels and produces an output label.
     */
    static std::string build_filter(const Params& params, const std::string& input_label, const std::string& output_label);

private:
    static std::string build_fade(const Params& params, const std::string& input_label, const std::string& output_label);
    static std::string build_wipe(const Params& params, const std::string& input_label, const std::string& output_label);
    static std::string build_circle(const Params& params, const std::string& input_label, const std::string& output_label);
};

} // namespace tachyon::media
