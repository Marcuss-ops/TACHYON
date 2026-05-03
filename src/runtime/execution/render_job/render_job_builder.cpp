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
    job.output.profile.name = "png-sequence";
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
    job.output.profile.name = "h264-mp4";
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

} // namespace tachyon
