#pragma once

#include "tachyon/core/render_graph/render_graph.h"
#include <string>

namespace tachyon::core {

/**
 * @brief Orchestrates the end-to-end media processing pipeline.
 * Derived from ruststream-core/core/render_graph/process.rs
 * 
 * The orchestrator is responsible for walking through each stage 
 * of the media pipeline, collecting metrics, and handling errors/fallbacks.
 */
class PipelineOrchestrator {
public:
    /**
     * @brief Executes the full pipeline for a given RenderGraph.
     * @param graph The render graph to execute.
     * @return A RenderResult containing the outcome and telemetry.
     */
    static RenderResult run(const RenderGraph& graph);

private:
    /**
     * @brief Stage: Validation of inputs and configuration.
     */
    static MediaResult<void> stage_validate(const RenderGraph& graph, RenderResult& result);
    
    /**
     * @brief Stage: Probing of source media metadata.
     */
    static MediaResult<void> stage_probe(const RenderGraph& graph, RenderResult& result);
    
    /**
     * @brief Stage: Decoding and preparation of source clips.
     */
    static MediaResult<void> stage_decode(const RenderGraph& graph, RenderResult& result);
    
    /**
     * @brief Stage: Application of video effects.
     */
    static MediaResult<void> stage_effects(const RenderGraph& graph, RenderResult& result);
    
    /**
     * @brief Stage: Merging of overlays and subtitles.
     */
    static MediaResult<void> stage_overlay(const RenderGraph& graph, RenderResult& result);
    
    /**
     * @brief Stage: Master audio mixdown (baking).
     */
    static MediaResult<void> stage_audio(const RenderGraph& graph, RenderResult& result);
    
    /**
     * @brief Stage: Final encoding and muxing.
     */
    static MediaResult<void> stage_encode(const RenderGraph& graph, RenderResult& result);
    
    /**
     * @brief Helper to update component-level telemetry.
     */
    static void update_metrics(RenderResult& result, const std::string& stage, bool success, bool fallback = false);
};

} // namespace tachyon::core
