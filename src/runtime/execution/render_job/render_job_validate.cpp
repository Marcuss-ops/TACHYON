#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"

namespace tachyon {

namespace {
bool is_quality_tier_valid(const std::string& tier) {
    return tier == "draft" || tier == "high" || tier == "cinematic";
}

bool is_alpha_mode_valid(const std::string& mode) {
    return mode == "premultiplied" || mode == "straight" || mode == "opaque";
}

bool is_motion_blur_curve_valid(const std::string& curve) {
    return curve == "box" || curve == "triangle" || curve == "gaussian";
}
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
