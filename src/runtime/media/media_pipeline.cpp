#include "tachyon/runtime/media/media_pipeline.h"
#include "tachyon/runtime/media/media_job_context.h"
#include "tachyon/runtime/media/media_services.h"
#include "tachyon/core/media/probe.h"
#include "tachyon/core/media/clip_processor.h"
#include "tachyon/core/media/overlay_merger.h"
#include "tachyon/core/media/video_concat.h"
#include "tachyon/core/media/audio_analyzer.h"
#include "tachyon/core/media/audio_extract.h"
#include "tachyon/core/audio_graph/audio_graph.h"
#include "tachyon/core/platform/process.h"

#include <chrono>
#include <filesystem>
#include <future>

namespace tachyon::runtime::media {

using namespace core::media;

core::RenderResult MediaPipeline::run(const core::RenderGraph& graph, MediaServices& services) {
    core::RenderResult result;
    result.success = true;
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Create Job Context
    auto job_ctx = MediaJobContext::create_default(graph.timeline->output.path);
    if (!std::filesystem::exists(job_ctx.work_dir)) {
        std::filesystem::create_directories(job_ctx.work_dir);
    }

    // Stage 1: Validate
    auto res_validate = stage_validate(graph, result, job_ctx, services);
    if (!res_validate.ok()) return core::RenderResult::failure(*res_validate.error);
    
    // Stage 2: Probe
    auto res_probe = stage_probe(graph, result, job_ctx, services);
    if (!res_probe.ok()) return core::RenderResult::failure(*res_probe.error);
    
    // Stage 3: Audio
    auto res_audio = stage_audio(graph, result, job_ctx, services);
    if (!res_audio.ok()) return core::RenderResult::failure(*res_audio.error);

    // Stage 3.5: Transcription
    auto res_transcribe = stage_transcribe(graph, result, job_ctx, services);
    if (!res_transcribe.ok()) return core::RenderResult::failure(*res_transcribe.error);

    // Stage 4: Decode / Prepare Clips
    auto res_decode = stage_decode(graph, result, job_ctx, services);
    if (!res_decode.ok()) return core::RenderResult::failure(*res_decode.error);
    
    // Stage 5: Effects
    auto res_effects = stage_effects(graph, result, job_ctx, services);
    if (!res_effects.ok()) return core::RenderResult::failure(*res_effects.error);
    
    // Stage 6: Overlay & Final Mux
    auto res_overlay = stage_overlay(graph, result, job_ctx, services);
    if (!res_overlay.ok()) return core::RenderResult::failure(*res_overlay.error);
    
    // Stage 7: Encode (Cleanup)
    auto res_encode = stage_encode(graph, result, job_ctx, services);
    if (!res_encode.ok()) return core::RenderResult::failure(*res_encode.error);
    
    auto end_time = std::chrono::steady_clock::now();
    result.drift = std::chrono::duration<double>(end_time - start_time).count();
    
    result.artifact_path = graph.timeline->output.path;
    return result;
}

std::future<core::RenderResult> MediaPipeline::run_async(const core::RenderGraph& graph, MediaServices& services) {
    return std::async(std::launch::async, [&graph, &services]() {
        return run(graph, services);
    });
}

void MediaPipeline::update_metrics(core::RenderResult& result, const std::string& stage, bool success, bool fallback) {
    auto& m = result.metrics[stage];
    m.attempts++;
    if (success) m.successes++;
    if (fallback) {
        m.fallbacks++;
        result.fallback_used = true;
        result.fallback_components.push_back(stage);
    }
}

core::MediaResult<void> MediaPipeline::stage_validate(const core::RenderGraph& graph, core::RenderResult& result, const MediaJobContext&, MediaServices&) {
    update_metrics(result, "validate", true);
    if (!graph.validate()) {
        return core::MediaResult<void>::failure(core::MediaError(core::MediaErrorCode::Timeline, "Graph validation failed"));
    }
    return core::MediaResult<void>::success();
}

core::MediaResult<void> MediaPipeline::stage_probe(const core::RenderGraph& graph, core::RenderResult& result, const MediaJobContext&, MediaServices& services) {
    update_metrics(result, "probe", true);
    
    for (const auto& track : graph.timeline->video_tracks) {
        for (const auto& segment : track.segments) {
            auto probe_res = services.probe.probe_file(segment.path);
            if (!probe_res.ok()) {
                return core::MediaResult<void>::failure(probe_res.error->with_stage("probe_segment"));
            }
        }
    }
    
    for (const auto& overlay : graph.timeline->overlays) {
        auto probe_res = services.probe.probe_file(overlay.path);
        if (!probe_res.ok()) {
            return core::MediaResult<void>::failure(probe_res.error->with_stage("probe_overlay"));
        }
    }
    
    return core::MediaResult<void>::success();
}

core::MediaResult<void> MediaPipeline::stage_decode(const core::RenderGraph& graph, core::RenderResult& result, const MediaJobContext& ctx, MediaServices& services) {
    update_metrics(result, "decode", true);
    
    if (graph.config.mode == core::RenderMode::Fast) {
        return core::MediaResult<void>::success();
    }
    
    for (const auto& track : graph.timeline->video_tracks) {
        for (size_t i = 0; i < track.segments.size(); ++i) {
            const auto& segment = track.segments[i];
            
            core::media::ClipProcessingConfig config;
            config.input_path = segment.path;
            config.output_path = ctx.work_dir / ("segment_" + std::to_string(i) + ".mp4");
            config.width = (int)graph.timeline->output.width;
            config.height = (int)graph.timeline->output.height;
            config.fps = graph.timeline->output.fps;
            config.crf = (int)graph.timeline->output.crf;
            
            auto proc_res = services.clip_processor.process_clip(config);
            if (!proc_res.ok()) {
                return core::MediaResult<void>::failure(proc_res.error->with_stage("decode_clip"));
            }
        }
    }
    
    return core::MediaResult<void>::success();
}

core::MediaResult<void> MediaPipeline::stage_effects(const core::RenderGraph&, core::RenderResult& result, const MediaJobContext&, MediaServices&) {
    update_metrics(result, "effects", true);
    return core::MediaResult<void>::success();
}

core::MediaResult<void> MediaPipeline::stage_audio(const core::RenderGraph& graph, core::RenderResult& result, const MediaJobContext& ctx, MediaServices& services) {
    update_metrics(result, "audio", true);
    
    const auto* primary_track = graph.timeline->find_primary_track();
    if (!primary_track || primary_track->segments.empty()) {
        return core::MediaResult<void>::success();
    }

    core::media::AudioExtractConfig config;
    config.input_file = primary_track->segments[0].path;
    config.output_wav = ctx.work_dir / "audio_extracted.wav";
    config.sample_rate = graph.timeline->output.sample_rate;
    config.channels = 2;

    return services.audio_extractor.extract(config);
}

core::MediaResult<void> MediaPipeline::stage_transcribe(const core::RenderGraph&, core::RenderResult& result, const MediaJobContext& ctx, MediaServices& services) {
    update_metrics(result, "transcribe", true);
    
    std::filesystem::path audio_path = ctx.work_dir / "audio_extracted.wav";
    if (!std::filesystem::exists(audio_path)) {
        return core::MediaResult<void>::success();
    }

    auto trans_res = services.audio_analyzer.transcribe(audio_path);
    if (!trans_res.ok()) {
        return core::MediaResult<void>::failure(trans_res.error->with_stage("transcribe"));
    }

    return core::MediaResult<void>::success();
}

core::MediaResult<void> MediaPipeline::stage_overlay(const core::RenderGraph& graph, core::RenderResult& result, const MediaJobContext& ctx, MediaServices& services) {
    update_metrics(result, "overlay", true);
    
    if (graph.timeline->overlays.empty()) {
        return core::MediaResult<void>::success();
    }

    core::media::OverlayMergeConfig config;
    
    const auto* primary_track = graph.timeline->find_primary_track();
    if (primary_track && !primary_track->segments.empty()) {
        std::filesystem::path temp_base = ctx.work_dir / "segment_0.mp4";
        if (std::filesystem::exists(temp_base)) {
            config.base_video_path = temp_base;
        } else {
            config.base_video_path = primary_track->segments[0].path;
        }
    } else {
        return core::MediaResult<void>::failure(core::MediaError(core::MediaErrorCode::Overlay, "No base video track found for overlays"));
    }

    config.output_path = graph.timeline->output.path;
    config.width = (int)graph.timeline->output.width;
    config.height = (int)graph.timeline->output.height;
    config.fps = graph.timeline->output.fps;
    config.crf = (int)graph.timeline->output.crf;
    
    for (const auto& ovl : graph.timeline->overlays) {
        core::media::OverlaySpec spec;
        spec.path = ovl.path;
        spec.start_time = graph.timeline->timebase.to_seconds(ovl.start_time);
        spec.duration = graph.timeline->timebase.to_seconds(ovl.duration);
        spec.x = (int)ovl.x;
        spec.y = (int)ovl.y;
        spec.opacity = ovl.opacity;
        config.overlays.push_back(spec);
    }
    
    auto merge_res = services.overlay_merger.merge_overlays(config);
    if (!merge_res.ok()) {
        return core::MediaResult<void>::failure(merge_res.error->with_stage("overlay_merge"));
    }
    
    return core::MediaResult<void>::success();
}

core::MediaResult<void> MediaPipeline::stage_encode(const core::RenderGraph&, core::RenderResult& result, const MediaJobContext& ctx, MediaServices&) {
    update_metrics(result, "encode", true);
    
    if (!ctx.keep_intermediates && std::filesystem::exists(ctx.work_dir)) {
        std::filesystem::remove_all(ctx.work_dir);
    }
    
    return core::MediaResult<void>::success();
}

} // namespace tachyon::runtime::media
