#include "tachyon/runtime/execution/render_job.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

namespace tachyon {
namespace {

using json = nlohmann::json;

void flatten_variables(
    const json& value,
    const std::string& prefix,
    std::unordered_map<std::string, double>& numeric_variables,
    std::unordered_map<std::string, std::string>& string_variables) {
    if (value.is_object()) {
        for (const auto& [key, child] : value.items()) {
            const std::string next_prefix = prefix.empty() ? key : prefix + "." + key;
            flatten_variables(child, next_prefix, numeric_variables, string_variables);
        }
        return;
    }

    if (value.is_number()) {
        numeric_variables[prefix] = value.get<double>();
        return;
    }

    if (value.is_string()) {
        string_variables[prefix] = value.get<std::string>();
        return;
    }
}

bool read_string(const json& object, const char* key, std::string& out) {
    if (!object.contains(key) || object.at(key).is_null()) {
        return false;
    }
    const auto& value = object.at(key);
    if (!value.is_string()) {
        return false;
    }
    out = value.get<std::string>();
    return true;
}

bool read_bool(const json& object, const char* key, bool& out) {
    if (!object.contains(key) || object.at(key).is_null()) {
        return false;
    }
    const auto& value = object.at(key);
    if (!value.is_boolean()) {
        return false;
    }
    out = value.get<bool>();
    return true;
}

bool read_int(const json& object, const char* key, std::int64_t& out) {
    if (!object.contains(key) || object.at(key).is_null()) {
        return false;
    }
    const auto& value = object.at(key);
    if (!value.is_number_integer()) {
        return false;
    }
    out = value.get<std::int64_t>();
    return true;
}

bool read_double(const json& object, const char* key, double& out) {
    if (!object.contains(key) || object.at(key).is_null()) {
        return false;
    }
    const auto& value = object.at(key);
    if (!value.is_number()) {
        return false;
    }
    out = value.get<double>();
    return true;
}

std::string make_path(const std::string& parent, const std::string& child) {
    if (parent.empty()) {
        return child;
    }
    if (child.empty()) {
        return parent;
    }
    return parent + "." + child;
}

OutputProfile parse_output_profile(const json& object, const std::string& path, DiagnosticBag& diagnostics) {
    OutputProfile profile;
    read_string(object, "name", profile.name);
    read_string(object, "class", profile.class_name);
    read_string(object, "container", profile.container);
    read_string(object, "alpha_mode", profile.alpha_mode);

    if (object.contains("video") && object.at("video").is_object()) {
        const auto& video = object.at("video");
        read_string(video, "codec", profile.video.codec);
        read_string(video, "pixel_format", profile.video.pixel_format);
        read_string(video, "rate_control_mode", profile.video.rate_control_mode);
        if (video.contains("crf") && video.at("crf").is_number()) {
            profile.video.crf = video.at("crf").get<double>();
        }
    } else {
        diagnostics.add_error("job.output.video_missing", "output.profile.video is required", make_path(path, "video"));
    }

    if (object.contains("audio") && object.at("audio").is_object()) {
        const auto& audio = object.at("audio");
        read_string(audio, "mode", profile.audio.mode);
        read_string(audio, "codec", profile.audio.codec);
        if (audio.contains("sample_rate")) {
            std::int64_t sample_rate{};
            if (read_int(audio, "sample_rate", sample_rate)) {
                profile.audio.sample_rate = sample_rate;
            }
        }
        if (audio.contains("channels")) {
            std::int64_t channels{};
            if (read_int(audio, "channels", channels)) {
                profile.audio.channels = channels;
            }
        }
    }

    if (object.contains("buffering") && object.at("buffering").is_object()) {
        const auto& buffering = object.at("buffering");
        read_string(buffering, "strategy", profile.buffering.strategy);
        if (buffering.contains("max_frames_in_queue")) {
            std::int64_t max_frames{};
            if (read_int(buffering, "max_frames_in_queue", max_frames)) {
                profile.buffering.max_frames_in_queue = max_frames;
            }
        }
    }

    if (object.contains("color") && object.at("color").is_object()) {
        const auto& color = object.at("color");
        read_string(color, "space", profile.color.space);
        read_string(color, "transfer", profile.color.transfer);
        read_string(color, "range", profile.color.range);
    }

    return profile;
}

bool is_quality_tier_valid(const std::string& tier) {
    return tier == "draft" || tier == "high" || tier == "cinematic";
}

bool is_alpha_mode_valid(const std::string& mode) {
    return mode == "premultiplied" || mode == "straight" || mode == "opaque";
}

bool is_motion_blur_curve_valid(const std::string& curve) {
    return curve == "box" || curve == "triangle" || curve == "gaussian";
}

} // namespace

ParseResult<RenderJob> parse_render_job_json(const std::string& text) {
    ParseResult<RenderJob> result;

    json root;
    try {
        root = json::parse(text);
    } catch (const json::parse_error& error) {
        result.diagnostics.add_error("job.json.parse_failed", error.what());
        return result;
    }

    if (!root.is_object()) {
        result.diagnostics.add_error("job.json.root_invalid", "render job root must be an object");
        return result;
    }

    RenderJob job;
    read_string(root, "job_id", job.job_id);
    read_string(root, "scene_ref", job.scene_ref);
    read_string(root, "composition_target", job.composition_target);
    if (root.contains("quality_tier") && root.at("quality_tier").is_string()) {
        job.quality_tier = root.at("quality_tier").get<std::string>();
    }
    if (root.contains("compositing_alpha_mode") && root.at("compositing_alpha_mode").is_string()) {
        job.compositing_alpha_mode = root.at("compositing_alpha_mode").get<std::string>();
    }
    if (root.contains("working_space") && root.at("working_space").is_string()) {
        job.working_space = root.at("working_space").get<std::string>();
    }
    read_string(root, "seed_policy_mode", job.seed_policy_mode);
    read_string(root, "compatibility_mode", job.compatibility_mode);
    if (root.contains("motion_blur_enabled") && root.at("motion_blur_enabled").is_boolean()) {
        job.motion_blur_enabled = root.at("motion_blur_enabled").get<bool>();
    }
    if (root.contains("motion_blur_samples") && root.at("motion_blur_samples").is_number_integer()) {
        job.motion_blur_samples = root.at("motion_blur_samples").get<std::int64_t>();
    }
    if (root.contains("motion_blur_shutter_angle") && root.at("motion_blur_shutter_angle").is_number()) {
        job.motion_blur_shutter_angle = root.at("motion_blur_shutter_angle").get<double>();
    }
    if (root.contains("motion_blur_curve") && root.at("motion_blur_curve").is_string()) {
        job.motion_blur_curve = root.at("motion_blur_curve").get<std::string>();
    }

    if (root.contains("frame_range") && root.at("frame_range").is_object()) {
        const auto& frame_range = root.at("frame_range");
        read_int(frame_range, "start", job.frame_range.start);
        read_int(frame_range, "end", job.frame_range.end);
    }

    if (root.contains("output") && root.at("output").is_object()) {
        const auto& output = root.at("output");
        if (output.contains("destination") && output.at("destination").is_object()) {
            const auto& destination = output.at("destination");
            read_string(destination, "path", job.output.destination.path);
            read_bool(destination, "overwrite", job.output.destination.overwrite);
        } else {
            result.diagnostics.add_error("job.output.destination_missing", "output.destination is required", "output.destination");
        }

        if (output.contains("profile") && output.at("profile").is_object()) {
            job.output.profile = parse_output_profile(output.at("profile"), "output.profile", result.diagnostics);
        } else {
            result.diagnostics.add_error("job.output.profile_missing", "output.profile is required", "output.profile");
        }
    } else {
        result.diagnostics.add_error("job.output.missing", "output object is required", "output");
    }

    if (root.contains("variables") && root.at("variables").is_object()) {
        flatten_variables(root.at("variables"), "", job.variables, job.string_variables);
    }

    if (result.diagnostics.ok()) {
        result.value = std::move(job);
    }
    return result;
}

ParseResult<RenderJob> parse_render_job_file(const std::filesystem::path& path) {
    ParseResult<RenderJob> result;
    std::ifstream input(path, std::ios::in | std::ios::binary);
    if (!input.is_open()) {
        result.diagnostics.add_error("job.file.open_failed", "failed to open render job file: " + path.string(), path.string());
        return result;
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    return parse_render_job_json(buffer.str());
}

ValidationResult validate_render_job(const RenderJob& job) {
    ValidationResult result;

    if (job.job_id.empty()) {
        result.diagnostics.add_error("job.job_id_missing", "job_id is required", "job_id");
    }

    if (job.scene_ref.empty()) {
        result.diagnostics.add_error("job.scene_ref_missing", "scene_ref is required", "scene_ref");
    }

    if (job.composition_target.empty()) {
        result.diagnostics.add_error("job.composition_target_missing", "composition_target is required", "composition_target");
    }

    if (job.quality_tier.empty()) {
        result.diagnostics.add_error("job.quality_tier_missing", "quality_tier is required", "quality_tier");
    } else if (!is_quality_tier_valid(job.quality_tier)) {
        result.diagnostics.add_error("job.quality_tier_invalid", "quality_tier must be draft, high, or cinematic", "quality_tier");
    }

    if (job.compositing_alpha_mode.empty()) {
        result.diagnostics.add_error("job.compositing_alpha_mode_missing", "compositing_alpha_mode is required", "compositing_alpha_mode");
    } else if (!is_alpha_mode_valid(job.compositing_alpha_mode)) {
        result.diagnostics.add_error(
            "job.compositing_alpha_mode_invalid",
            "compositing_alpha_mode must be premultiplied, straight, or opaque",
            "compositing_alpha_mode");
    }

    if (job.working_space.empty()) {
        result.diagnostics.add_error("job.working_space_missing", "working_space is required", "working_space");
    }

    if (job.motion_blur_enabled) {
        if (job.motion_blur_shutter_angle < 0.0 || job.motion_blur_shutter_angle > 360.0) {
            result.diagnostics.add_error(
                "job.motion_blur_shutter_angle_invalid",
                "motion_blur_shutter_angle must be between 0 and 360",
                "motion_blur_shutter_angle");
        }

        if (!is_motion_blur_curve_valid(job.motion_blur_curve)) {
            result.diagnostics.add_error(
                "job.motion_blur_curve_invalid",
                "motion_blur_curve must be box, triangle, or gaussian",
                "motion_blur_curve");
        }
    }

    if (job.frame_range.end < job.frame_range.start) {
        result.diagnostics.add_error("job.frame_range_invalid", "frame_range.end must be greater than or equal to frame_range.start", "frame_range");
    }

    if (job.output.destination.path.empty()) {
        result.diagnostics.add_error("job.output.path_missing", "output.destination.path is required", "output.destination.path");
    }

    if (job.output.profile.container.empty()) {
        result.diagnostics.add_error("job.output.container_missing", "output.profile.container is required", "output.profile.container");
    }

    if (job.output.profile.name.empty()) {
        result.diagnostics.add_error("job.output.profile_name_missing", "output.profile.name is required", "output.profile.name");
    }

    if (job.output.profile.class_name.empty()) {
        result.diagnostics.add_error("job.output.profile_class_missing", "output.profile.class is required", "output.profile.class");
    }

    if (job.output.profile.video.codec.empty()) {
        result.diagnostics.add_error("job.output.video_codec_missing", "output.profile.video.codec is required", "output.profile.video.codec");
    }

    if (job.output.profile.video.pixel_format.empty()) {
        result.diagnostics.add_error("job.output.pixel_format_missing", "output.profile.video.pixel_format is required", "output.profile.video.pixel_format");
    }

    if (job.output.profile.video.rate_control_mode.empty()) {
        result.diagnostics.add_error("job.output.rate_control_mode_missing", "output.profile.video.rate_control_mode is required", "output.profile.video.rate_control_mode");
    }

    if (job.output.profile.buffering.strategy.empty()) {
        result.diagnostics.add_error("job.output.buffering_missing", "output.profile.buffering.strategy is required", "output.profile.buffering.strategy");
    }

    if (job.output.profile.color.transfer.empty()) {
        result.diagnostics.add_error("job.output.color_transfer_missing", "output.profile.color.transfer is required", "output.profile.color.transfer");
    }

    if (job.output.profile.color.range.empty()) {
        result.diagnostics.add_error("job.output.color_range_missing", "output.profile.color.range is required", "output.profile.color.range");
    }

    if (job.output.profile.color.space.empty()) {
        result.diagnostics.add_error("job.output.color_space_missing", "output.profile.color.space is required", "output.profile.color.space");
    }

    if (job.output.profile.alpha_mode.empty()) {
        result.diagnostics.add_error("job.output.alpha_mode_missing", "output.profile.alpha_mode is required", "output.profile.alpha_mode");
    } else if (job.output.profile.alpha_mode != "discarded" &&
               job.output.profile.alpha_mode != "preserved" &&
               job.output.profile.alpha_mode != "unsupported") {
        result.diagnostics.add_error("job.output.alpha_mode_invalid", "output.profile.alpha_mode must be discarded, preserved, or unsupported", "output.profile.alpha_mode");
    }

    if (!job.seed_policy_mode.empty() &&
        job.seed_policy_mode != "stable" &&
        job.seed_policy_mode != "deterministic" &&
        job.seed_policy_mode != "random") {
        result.diagnostics.add_error("job.seed_policy_mode_invalid", "seed_policy_mode must be stable, deterministic, or random", "seed_policy_mode");
    }

    if (!job.compatibility_mode.empty() &&
        job.compatibility_mode != "locked_v1" &&
        job.compatibility_mode != "legacy" &&
        job.compatibility_mode != "current") {
        result.diagnostics.add_warning("job.compatibility_mode_unrecognized", "compatibility_mode should be one of locked_v1, legacy, or current", "compatibility_mode");
    }

    if (!job.output.profile.audio.mode.empty()) {
        const auto& mode = job.output.profile.audio.mode;
        if (mode != "none" && mode != "passthrough" && mode != "encode") {
            result.diagnostics.add_error("job.output.audio_mode_invalid", "audio mode must be none, passthrough, or encode", "output.profile.audio.mode");
        }
        if (mode == "encode") {
            if (job.output.profile.audio.codec.empty()) {
                result.diagnostics.add_error("job.output.audio_codec_missing", "output.profile.audio.codec is required when audio.mode is encode", "output.profile.audio.codec");
            }
            if (!job.output.profile.audio.sample_rate.has_value()) {
                result.diagnostics.add_error("job.output.audio_sample_rate_missing", "output.profile.audio.sample_rate is required when audio.mode is encode", "output.profile.audio.sample_rate");
            }
            if (!job.output.profile.audio.channels.has_value()) {
                result.diagnostics.add_error("job.output.audio_channels_missing", "output.profile.audio.channels is required when audio.mode is encode", "output.profile.audio.channels");
            }
        }
    }

    return result;
}

} // namespace tachyon
