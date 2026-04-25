#include "render_job_internal.h"
#include <fstream>
#include <sstream>

namespace tachyon {

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
            if (read_number(audio, "sample_rate", sample_rate)) {
                profile.audio.sample_rate = sample_rate;
            }
        }
        if (audio.contains("channels")) {
            std::int64_t channels{};
            if (read_number(audio, "channels", channels)) {
                profile.audio.channels = channels;
            }
        }
    }

    if (object.contains("buffering") && object.at("buffering").is_object()) {
        const auto& buffering = object.at("buffering");
        read_string(buffering, "strategy", profile.buffering.strategy);
        if (buffering.contains("max_frames_in_queue")) {
            std::int64_t max_frames{};
            if (read_number(buffering, "max_frames_in_queue", max_frames)) {
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
    if (root.contains("proxy_enabled") && root.at("proxy_enabled").is_boolean()) {
        job.proxy_enabled = root.at("proxy_enabled").get<bool>();
    }
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
        read_number(frame_range, "start", job.frame_range.start);
        read_number(frame_range, "end", job.frame_range.end);
    }

    if (root.contains("output") && root.at("output").is_object()) {
        const auto& output = root.at("output");
        if (output.contains("destination") && output.at("destination").is_object()) {
            const auto& destination = output.at("destination");
            read_string(destination, "path", job.output.destination.path);
            read_bool(destination, "overwrite", job.output.destination.overwrite);
        }

        if (output.contains("profile") && output.at("profile").is_object()) {
            job.output.profile = parse_output_profile(output.at("profile"), "output.profile", result.diagnostics);
        }
    }

    if (root.contains("variables") && root.at("variables").is_object()) {
        flatten_variables(root.at("variables"), "", job.variables, job.string_variables);
    }

    if (result.diagnostics.ok()) result.value = std::move(job);
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

} // namespace tachyon
