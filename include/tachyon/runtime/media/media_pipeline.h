#pragma once

#include "tachyon/core/render_graph/render_graph.h"
#include "tachyon/runtime/media/media_job_context.h"
#include "tachyon/runtime/media/media_services.h"
#include <string>
#include <future>

namespace tachyon::runtime::media {

/**
 * @brief Orchestrates the end-to-end media processing pipeline.
 * Derived from ruststream-core/core/render_graph/process.rs
 * 
 * The pipeline is responsible for walking through each stage 
 * of the media processing flow, collecting metrics, and handling errors/fallbacks.
 */
class MediaPipeline {
public:
    /**
     * @brief Executes the full pipeline for a given RenderGraph.
     * @param graph The render graph to execute.
     * @param services The media services provider.
     * @return A RenderResult containing the outcome and telemetry.
     */
    static core::RenderResult run(const core::RenderGraph& graph, MediaServices& services);

    /**
     * @brief Executes the full pipeline asynchronously.
     * @param graph The render graph to execute.
     * @param services The media services provider.
     * @return A future that will eventually contain the RenderResult.
     */
    static std::future<core::RenderResult> run_async(const core::RenderGraph& graph, MediaServices& services);

private:
    /**
     * @brief Stage: Validation of inputs and configuration.
     */
    static core::MediaResult<void> stage_validate(const core::RenderGraph& graph, core::RenderResult& result, const MediaJobContext& ctx, MediaServices& services);
    
    /**
     * @brief Stage: Probing of source media metadata.
     */
    static core::MediaResult<void> stage_probe(const core::RenderGraph& graph, core::RenderResult& result, const MediaJobContext& ctx, MediaServices& services);
    
    /**
     * @brief Stage: Decoding and preparation of source clips.
     */
    static core::MediaResult<void> stage_decode(const core::RenderGraph& graph, core::RenderResult& result, const MediaJobContext& ctx, MediaServices& services);
    
    /**
     * @brief Stage: Application of video effects.
     */
    static core::MediaResult<void> stage_effects(const core::RenderGraph& graph, core::RenderResult& result, const MediaJobContext& ctx, MediaServices& services);
    
    /**
     * @brief Stage: Merging of overlays and subtitles.
     */
    static core::MediaResult<void> stage_overlay(const core::RenderGraph& graph, core::RenderResult& result, const MediaJobContext& ctx, MediaServices& services);
    
    /**
     * @brief Stage: Master audio mixdown (baking).
     */
    static core::MediaResult<void> stage_audio(const core::RenderGraph& graph, core::RenderResult& result, const MediaJobContext& ctx, MediaServices& services);
 
    /**
     * @brief Stage: Audio transcription and STT.
     */
    static core::MediaResult<void> stage_transcribe(const core::RenderGraph& graph, core::RenderResult& result, const MediaJobContext& ctx, MediaServices& services);
    
    /**
     * @brief Stage: Final encoding and muxing.
     */
    static core::MediaResult<void> stage_encode(const core::RenderGraph& graph, core::RenderResult& result, const MediaJobContext& ctx, MediaServices& services);
    
    /**
     * @brief Helper to update component-level telemetry.
     */
    static void update_metrics(core::RenderResult& result, const std::string& stage, bool success, bool fallback = false);
};

} // namespace tachyon::runtime::media
