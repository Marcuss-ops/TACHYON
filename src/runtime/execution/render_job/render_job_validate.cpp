#include "render_job_internal.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"

namespace tachyon {

ValidationResult validate_render_job(const RenderJob& job) {
    ValidationResult result;

    if (job.job_id.empty()) {
        result.diagnostics.add_error("job.job_id_missing", "job_id is required", "job_id");
    }

    if (job.scene_ref.empty()) {
        result.diagnostics.add_error("job.scene_ref_missing", "scene_ref is required", "scene_ref");
    } else {
        // Check for missing font manifest when text layers are present.
        // Load the scene spec to inspect.
        auto scene_result = parse_scene_spec_file(std::filesystem::path(job.scene_ref));
        if (scene_result.value.has_value()) {
            const auto& scene = *scene_result.value;
            if (!scene.font_manifest.has_value()) {
                bool has_text = false;
                for (const auto& comp : scene.compositions) {
                    for (const auto& layer : comp.layers) {
                        if (layer.type == "text") {
                            has_text = true;
                            break;
                        }
                    }
                    if (has_text) break;
                }
                if (has_text) {
                    result.diagnostics.add_warning(
                        "job.font_manifest.missing",
                        "scene missing font_manifest but contains text layers; fonts may not load correctly"
                    );
                }
            }
        }
    }

    if (job.composition_target.empty()) {
        result.diagnostics.add_error("job.composition_target_missing", "composition_target is required", "composition_target");
    }

    if (!job.quality_tier.empty() && !is_quality_tier_valid(job.quality_tier)) {
        result.diagnostics.add_error("job.quality_tier_invalid", "quality_tier must be draft, high, or cinematic", "quality_tier");
    }

    if (!job.compositing_alpha_mode.empty() && !is_alpha_mode_valid(job.compositing_alpha_mode)) {
        result.diagnostics.add_error(
            "job.compositing_alpha_mode_invalid",
            "compositing_alpha_mode must be premultiplied, straight, or opaque",
            "compositing_alpha_mode");
    }

    if (!job.working_space.empty()) {
        // Validate if provided
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

    if (!job.output.destination.path.empty()) {
        // Validate if provided
    }

    if (!job.output.profile.container.empty()) {
        // Validate if provided
    }

    if (!job.output.profile.name.empty()) {
        // Validate if provided
    }

    if (!job.output.profile.class_name.empty()) {
        // Validate if provided
    }

    if (!job.output.profile.video.codec.empty()) {
        // Validate if provided
    }

    if (!job.output.profile.video.pixel_format.empty()) {
        // Validate if provided
    }

    if (!job.output.profile.video.rate_control_mode.empty()) {
        // Validate if provided
    }

    if (!job.output.profile.buffering.strategy.empty()) {
        // Validate if provided
    }

    if (!job.output.profile.color.transfer.empty()) {
        // Validate if provided
    }

    if (!job.output.profile.color.range.empty()) {
        // Validate if provided
    }

    if (!job.output.profile.color.space.empty()) {
        // Validate if provided
    }

    if (!job.output.profile.alpha_mode.empty()) {
        if (job.output.profile.alpha_mode != "discarded" &&
                job.output.profile.alpha_mode != "preserved" &&
                job.output.profile.alpha_mode != "unsupported") {
            result.diagnostics.add_error("job.output.alpha_mode_invalid", "output.profile.alpha_mode must be discarded, preserved, or unsupported", "output.profile.alpha_mode");
        }
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
