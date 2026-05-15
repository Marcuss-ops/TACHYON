#include "tachyon/runtime/media/media_pipeline.h"
#include "tachyon/core/media/probe.h"
#include "tachyon/core/media/clip_processor.h"
#include "tachyon/core/media/overlay_merger.h"
#include "tachyon/core/media/video_concat.h"
#include "tachyon/core/media/audio_analyzer.h"
#include "tachyon/core/audio_graph/audio_graph.h"
#include "tachyon/core/platform/process.h"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <future>

namespace tachyon::runtime::media {

core::RenderResult MediaPipeline::run(const core::RenderGraph& graph) {
    core::RenderResult result;
    result.success = true;
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Stage 1: Validate
    auto res_validate = stage_validate(graph, result);
    if (!res_validate.ok()) return core::RenderResult::failure(*res_validate.error);
    
    // Stage 2: Probe
    auto res_probe = stage_probe(graph, result);
    if (!res_probe.ok()) return core::RenderResult::failure(*res_probe.error);
    
    // Stage 3: Audio (Prioritize audio extraction for faster results)
    auto res_audio = stage_audio(graph, result);
    if (!res_audio.ok()) return core::RenderResult::failure(*res_audio.error);

    // Stage 3.5: Transcription (Whisper Analysis)
    auto res_transcribe = stage_transcribe(graph, result);
    if (!res_transcribe.ok()) return core::RenderResult::failure(*res_transcribe.error);

    // Stage 4: Decode / Prepare Clips (Video processing)
    auto res_decode = stage_decode(graph, result);
    if (!res_decode.ok()) return core::RenderResult::failure(*res_decode.error);
    
    // Stage 5: Effects (Internal Tachyon Renderer)
    auto res_effects = stage_effects(graph, result);
    if (!res_effects.ok()) return core::RenderResult::failure(*res_effects.error);
    
    // Stage 6: Overlay & Final Mux
    auto res_overlay = stage_overlay(graph, result);
    if (!res_overlay.ok()) return core::RenderResult::failure(*res_overlay.error);
    
    // Stage 7: Encode (Cleanup and Finalization)
    auto res_encode = stage_encode(graph, result);
    if (!res_encode.ok()) return core::RenderResult::failure(*res_encode.error);
    
    auto end_time = std::chrono::steady_clock::now();
    result.drift = std::chrono::duration<double>(end_time - start_time).count();
    
    result.artifact_path = graph.timeline->output.path;
    return result;
}

std::future<core::RenderResult> MediaPipeline::run_async(const core::RenderGraph& graph) {
    // Launch the run method in a separate thread
    return std::async(std::launch::async, [graph]() {
        return run(graph);
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

core::MediaResult<void> MediaPipeline::stage_validate(const core::RenderGraph& graph, core::RenderResult& result) {
    update_metrics(result, "validate", true);
    if (!graph.validate()) {
        return core::MediaResult<void>::failure(core::MediaError(core::MediaErrorCode::Timeline, "Graph validation failed"));
    }
    return core::MediaResult<void>::success();
}

core::MediaResult<void> MediaPipeline::stage_probe(const core::RenderGraph& graph, core::RenderResult& result) {
    update_metrics(result, "probe", true);
    
    // Probe all video segments
    for (const auto& track : graph.timeline->video_tracks) {
        for (const auto& segment : track.segments) {
            auto probe_res = core::media::MediaProbe::probe_file(segment.path);
            if (!probe_res.ok()) {
                return core::MediaResult<void>::failure(probe_res.error->with_stage("probe_segment"));
            }
        }
    }
    
    // Probe all overlays
    for (const auto& overlay : graph.timeline->overlays) {
        auto probe_res = core::media::MediaProbe::probe_file(overlay.path);
        if (!probe_res.ok()) {
            return core::MediaResult<void>::failure(probe_res.error->with_stage("probe_overlay"));
        }
    }
    
    return core::MediaResult<void>::success();
}

core::MediaResult<void> MediaPipeline::stage_decode(const core::RenderGraph& graph, core::RenderResult& result) {
    update_metrics(result, "decode", true);
    
    if (graph.config.mode == core::RenderMode::Fast) {
        std::cout << "[Runtime::Media] Fast Mode: Skipping video decode stage." << std::endl;
        return core::MediaResult<void>::success();
    }
    
    // Prepare each segment (transcode if necessary, apply fades)
    for (const auto& track : graph.timeline->video_tracks) {
        for (size_t i = 0; i < track.segments.size(); ++i) {
            const auto& segment = track.segments[i];
            
            core::media::ClipProcessingConfig config;
            config.input_path = segment.path;
            
            // Generate intermediate path
            std::filesystem::path inter_path = "temp_segment_" + std::to_string(i) + ".mp4";
            config.output_path = inter_path;
            
            config.width = graph.timeline->output.width;
            config.height = graph.timeline->output.height;
            config.fps = graph.timeline->output.fps;
            config.crf = graph.timeline->output.crf;
            
            auto proc_res = core::media::ClipProcessor::process_clip(config);
            if (!proc_res.ok()) {
                return core::MediaResult<void>::failure(proc_res.error->with_stage("decode_clip"));
            }
        }
    }
    
    return core::MediaResult<void>::success();
}

core::MediaResult<void> MediaPipeline::stage_effects(const core::RenderGraph& graph, core::RenderResult& result) {
    update_metrics(result, "effects", true);
    // TODO: Integrate TachyonRenderer2D for procedural effects
    return core::MediaResult<void>::success();
}

core::MediaResult<void> MediaPipeline::stage_audio(const core::RenderGraph& graph, core::RenderResult& result) {
    update_metrics(result, "audio", true);
    
    const auto* primary_track = graph.timeline->find_primary_track();
    if (!primary_track || primary_track->segments.empty()) {
        return core::MediaResult<void>::success(); // No audio to extract
    }

    // Extract audio from the first segment of the primary track as a test
    std::filesystem::path input_path = primary_track->segments[0].path;
    std::filesystem::path output_audio = graph.timeline->output.path + ".wav";

    std::vector<std::string> args = {
        "-y",
        "-i", input_path.string(),
        "-vn", // No video
        "-acodec", "pcm_s16le",
        "-ar", std::to_string(graph.timeline->output.sample_rate),
        "-ac", "2",
        output_audio.string()
    };

    ::tachyon::core::platform::ProcessSpec spec;
    spec.executable = "ffmpeg";
    spec.args = args;
    
    auto proc_res = ::tachyon::core::platform::run_process(spec);
    if (!proc_res.success || proc_res.exit_code != 0) {
        return core::MediaResult<void>::failure(
            core::MediaError(core::MediaErrorCode::Audio, "Audio extraction failed: " + proc_res.error)
        );
    }
    
    std::cout << "[Runtime::Media] Audio extracted successfully to: " << output_audio << std::endl;
    return core::MediaResult<void>::success();
}

core::MediaResult<void> MediaPipeline::stage_transcribe(const core::RenderGraph& graph, core::RenderResult& result) {
    update_metrics(result, "transcribe", true);
    
    // We transcribe the audio extracted in the previous stage
    std::filesystem::path audio_path = graph.timeline->output.path + ".wav";
    
    if (!std::filesystem::exists(audio_path)) {
        return core::MediaResult<void>::success(); // Nothing to transcribe
    }

    auto trans_res = core::media::AudioAnalyzer::transcribe(audio_path);
    if (!trans_res.ok()) {
        return core::MediaResult<void>::failure(trans_res.error->with_stage("transcribe"));
    }

    std::cout << "[Runtime::Media] Transcription completed: " << *trans_res << std::endl;
    return core::MediaResult<void>::success();
}

core::MediaResult<void> MediaPipeline::stage_overlay(const core::RenderGraph& graph, core::RenderResult& result) {
    update_metrics(result, "overlay", true);
    
    if (graph.timeline->overlays.empty()) {
        std::cout << "[Runtime::Media] No overlays to merge. Skipping overlay stage." << std::endl;
        return core::MediaResult<void>::success();
    }

    core::media::OverlayMergeConfig config;
    
    // Determine base video path: use prepared segment if available, otherwise original
    const auto* primary_track = graph.timeline->find_primary_track();
    if (primary_track && !primary_track->segments.empty()) {
        std::filesystem::path temp_base = "temp_segment_0.mp4";
        if (std::filesystem::exists(temp_base)) {
            config.base_video_path = temp_base;
        } else {
            config.base_video_path = primary_track->segments[0].path;
        }
    } else {
        return core::MediaResult<void>::failure(core::MediaError(core::MediaErrorCode::Overlay, "No base video track found for overlays"));
    }

    config.output_path = graph.timeline->output.path;
    config.width = graph.timeline->output.width;
    config.height = graph.timeline->output.height;
    config.fps = graph.timeline->output.fps;
    config.crf = graph.timeline->output.crf;
    
    for (const auto& ovl : graph.timeline->overlays) {
        core::media::OverlaySpec spec;
        spec.path = ovl.path;
        spec.start_time = graph.timeline->timebase.to_seconds(ovl.start_time);
        spec.duration = graph.timeline->timebase.to_seconds(ovl.duration);
        spec.x = ovl.x;
        spec.y = ovl.y;
        spec.opacity = ovl.opacity;
        config.overlays.push_back(spec);
    }
    
    auto merge_res = core::media::OverlayMerger::merge_overlays(config);
    if (!merge_res.ok()) {
        return core::MediaResult<void>::failure(merge_res.error->with_stage("overlay_merge"));
    }
    
    return core::MediaResult<void>::success();
}

core::MediaResult<void> MediaPipeline::stage_encode(const core::RenderGraph& graph, core::RenderResult& result) {
    update_metrics(result, "encode", true);
    // Cleanup intermediate files
    return core::MediaResult<void>::success();
}

} // namespace tachyon::runtime::media
