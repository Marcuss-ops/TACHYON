#include "tachyon/runtime/execution/jobs/render_job.h"

namespace tachyon {

RenderJob RenderJobBuilder::still_image(
    const std::string& composition_id, 
    std::int64_t frame, 
    const std::string& output_path) {
    
    RenderJob job;
    job.job_id = "still_render_" + composition_id + "_" + std::to_string(frame);
    job.composition_target = composition_id;
    job.frame_range = {frame, frame};
    job.output.destination.path = output_path;
    job.output.destination.overwrite = true;
    
    // Default to high quality PNG sequence (one frame)
    job.output.profile.name = "png_sequence";
    job.output.profile.container = "png";
    job.output.profile.format = OutputFormat::ImageSequence;
    job.output.profile.video.codec = "png";
    job.output.profile.video.pixel_format = "rgba8";
    job.output.profile.video.rate_control_mode = "fixed";
    job.output.profile.class_name = "image-sequence";
    job.output.profile.buffering.strategy = "default";
    job.output.profile.color.transfer = "srgb";
    job.output.profile.color.range = "full";
    job.output.profile.color.space = "bt709";
    job.output.profile.alpha_mode = "preserved";
    job.scene_ref = "builder";
    
    return job;
}

RenderJob RenderJobBuilder::video_export(
    const std::string& composition_id, 
    FrameRange range, 
    const std::string& output_path) {
    
    RenderJob job;
    job.job_id = "video_export_" + composition_id;
    job.composition_target = composition_id;
    job.frame_range = range;
    job.output.destination.path = output_path;
    job.output.destination.overwrite = true;
    
    // Default to H.264 MP4
    job.output.profile.name = "youtube_1080p_30";
    job.output.profile.container = "mp4";
    job.output.profile.format = OutputFormat::Video;
    job.output.profile.video.codec = "libx264";
    job.output.profile.video.pixel_format = "yuv420p";
    job.output.profile.video.rate_control_mode = "crf";
    job.output.profile.video.crf = 18.0;
    job.output.profile.class_name = "video-export";
    job.output.profile.buffering.strategy = "default";
    job.output.profile.color.transfer = "bt709";
    job.output.profile.color.range = "limited";
    job.output.profile.color.space = "bt709";
    job.output.profile.alpha_mode = "discarded";
    job.scene_ref = "builder";
    
    return job;
}

OutputProfile make_png_sequence_profile() {
    OutputProfile profile;
    profile.name = "png_sequence";
    profile.container = "png";
    profile.format = OutputFormat::ImageSequence;
    profile.video.codec = "png";
    profile.video.pixel_format = "rgba8";
    profile.video.rate_control_mode = "fixed";
    profile.class_name = "image-sequence";
    profile.buffering.strategy = "default";
    profile.color.transfer = "srgb";
    profile.color.range = "full";
    profile.color.space = "bt709";
    profile.alpha_mode = "preserved";
    return profile;
}

OutputProfile make_h264_mp4_profile() {
    OutputProfile profile;
    profile.name = "h264_high";
    profile.container = "mp4";
    profile.format = OutputFormat::Video;
    profile.video.codec = "libx264";
    profile.video.pixel_format = "yuv420p";
    profile.video.rate_control_mode = "crf";
    profile.video.crf = 18.0;
    profile.class_name = "video-export";
    profile.buffering.strategy = "default";
    profile.color.transfer = "bt709";
    profile.color.range = "limited";
    profile.color.space = "bt709";
    profile.alpha_mode = "discarded";
    return profile;
}

OutputProfile resolve_output_profile(const std::string& name) {
    if (name == "png_sequence" || name == "png") {
        return make_png_sequence_profile();
    } else if (name == "h264" || name == "mp4" || name == "video") {
        return make_h264_mp4_profile();
    }
    
    // Default to H.264
    OutputProfile profile = make_h264_mp4_profile();
    profile.name = name;
    apply_output_preset(profile);
    return profile;
}

} // namespace tachyon
