#include "tachyon/runtime/render_job.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

namespace tachyon {
namespace {

using json = nlohmann::json;

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
        read_string(color, "transfer", profile.color.transfer);
        read_string(color, "range", profile.color.range);
    }

    return profile;
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
    read_string(root, "seed_policy_mode", job.seed_policy_mode);
    read_string(root, "compatibility_mode", job.compatibility_mode);

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

    if (job.frame_range.end < job.frame_range.start) {
        result.diagnostics.add_error("job.frame_range_invalid", "frame_range.end must be greater than or equal to frame_range.start", "frame_range");
    }

    if (job.output.destination.path.empty()) {
        result.diagnostics.add_error("job.output.path_missing", "output.destination.path is required", "output.destination.path");
    }

    if (job.output.profile.container.empty()) {
        result.diagnostics.add_error("job.output.container_missing", "output.profile.container is required", "output.profile.container");
    }

    if (job.output.profile.video.codec.empty()) {
        result.diagnostics.add_error("job.output.video_codec_missing", "output.profile.video.codec is required", "output.profile.video.codec");
    }

    if (job.output.profile.buffering.strategy.empty()) {
        result.diagnostics.add_error("job.output.buffering_missing", "output.profile.buffering.strategy is required", "output.profile.buffering.strategy");
    }

    if (!job.output.profile.audio.mode.empty()) {
        const auto& mode = job.output.profile.audio.mode;
        if (mode != "none" && mode != "passthrough" && mode != "encode") {
            result.diagnostics.add_error("job.output.audio_mode_invalid", "audio mode must be none, passthrough, or encode", "output.profile.audio.mode");
        }
    }

    return result;
}

} // namespace tachyon
