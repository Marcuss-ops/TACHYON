#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/runtime/execution/frames/frame_executor.h"

#include "tachyon/output/frame_output_sink.h"
#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/renderer2d/resource/texture_resolver.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <memory>
#include <vector>

namespace tachyon {
namespace {

void render_frames_sequential(
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    FrameCache& cache,
    RenderContext& context,
    std::vector<ExecutedFrame>& rendered_frames) {
    
    rendered_frames.reserve(execution_plan.frame_tasks.size());
    FrameArena arena;
    FrameExecutor executor(arena, cache);

    for (const auto& task : execution_plan.frame_tasks) {
        rendered_frames.push_back(executor.execute(compiled_scene, execution_plan.render_plan, task, context));
        arena.reset();
    }
}

void render_frames_parallel(
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    FrameCache& cache,
    std::size_t worker_count,
    RenderContext& context,
    std::vector<ExecutedFrame>& rendered_frames) {

    const std::size_t task_count = execution_plan.frame_tasks.size();
    rendered_frames.resize(task_count);
    const std::size_t thread_count = std::max<std::size_t>(1, std::min(worker_count, task_count));
    std::atomic<std::size_t> next_index{0};
    std::vector<std::future<void>> workers;
    workers.reserve(thread_count);

    for (std::size_t worker_index = 0; worker_index < thread_count; ++worker_index) {
        workers.push_back(std::async(std::launch::async, [&]() {
            FrameArena arena;
            FrameExecutor executor(arena, cache);
            
            // Create a thread-local context that shares the heavy managers (media, ray_tracer)
            RenderContext local_context(context.renderer2d.precomp_cache);
            local_context.media = context.media;
            local_context.ray_tracer = context.ray_tracer;
            local_context.policy = context.policy;
            local_context.renderer2d.policy = context.policy;
            local_context.renderer2d.font_registry = context.renderer2d.font_registry;
            local_context.renderer2d.media_manager = context.media.get();
            
            for (;;) {
                const std::size_t index = next_index.fetch_add(1);
                if (index >= task_count) {
                    return;
                }

                rendered_frames[index] = executor.execute(compiled_scene, execution_plan.render_plan, execution_plan.frame_tasks[index], local_context);
                arena.reset();
            }
        }));
    }


    for (auto& worker : workers) {
        worker.get();
    }
}

} // namespace

RenderSessionResult RenderSession::render(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    const std::filesystem::path& output_path) {
    return render(scene, compiled_scene, execution_plan, output_path, 1);
}

RenderSessionResult RenderSession::render(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    const std::filesystem::path& output_path,
    std::size_t worker_count) {
    (void)scene; // SceneSpec is no longer used in the hot path.
    
    RenderSessionResult result;
    // get_default_text_font returns the font, we'll store it in the context later if needed
    
    RenderContext context(m_precomp_cache);
    context.policy = make_quality_policy(execution_plan.render_plan.quality_tier);
    context.renderer2d.policy = context.policy;
    context.renderer2d.font_registry = ::tachyon::renderer2d::get_default_font_registry();
    
    const std::size_t cache_budget_bytes = m_memory_budget_bytes.value_or(context.policy.precomp_cache_budget);
    m_cache.set_budget_bytes(cache_budget_bytes);

    if (context.renderer2d.precomp_cache) {
        context.renderer2d.precomp_cache->set_max_bytes(context.policy.precomp_cache_budget);
    }

    const std::size_t effective_worker_count = context.policy.max_workers > 0
        ? std::max<std::size_t>(1, std::min(worker_count, context.policy.max_workers))
        : worker_count;

    RenderPlan effective_plan = execution_plan.render_plan;
    if (!output_path.empty()) {
        effective_plan.output.destination.path = output_path.string();
    }

    std::unique_ptr<output::FrameOutputSink> sink = output::create_frame_output_sink(effective_plan);
    if (sink) {
        result.output_configured = true;
        if (!sink->begin(effective_plan)) {
            result.output_error = sink->last_error();
            return result;
        }
    }

    std::vector<std::string> active_video_paths;
    for (const auto& asset : compiled_scene.assets) {
        if (asset.type == "video") {
            active_video_paths.push_back(asset.path);
        }
    }
    const double session_fps = (compiled_scene.compositions.empty() || compiled_scene.compositions.front().fps == 0) 
        ? 60.0 
        : static_cast<double>(compiled_scene.compositions.front().fps);
    
    // Update media prefetcher with current time
    if (!execution_plan.frame_tasks.empty() && context.media) {
        m_prefetcher.update(*context.media, active_video_paths, execution_plan.frame_tasks.front().time_seconds, session_fps);
    }

    // Prepare evaluation plan
    
    std::vector<ExecutedFrame> rendered_frames;
    if (effective_worker_count <= 1 || execution_plan.frame_tasks.size() <= 1) {
        render_frames_sequential(compiled_scene, execution_plan, m_cache, context, rendered_frames);
    } else {
        render_frames_parallel(compiled_scene, execution_plan, m_cache, effective_worker_count, context, rendered_frames);
    }

    for (ExecutedFrame& frame : rendered_frames) {
        if (frame.cache_hit) {
            ++result.cache_hits;
        } else {
            ++result.cache_misses;
        }

        if (sink) {
            const output::OutputFramePacket packet{frame.frame_number, frame.frame.get()};
            if (!sink->write_frame(packet)) {
                result.output_error = sink->last_error();
            } else {
                ++result.frames_written;
            }
        }

        // Tier 1 Diagnostics Reporting
        const char* diag_env = std::getenv("TACHYON_DIAGNOSTICS");
        if (diag_env && std::string_view(diag_env) == "1") {
            const auto& d = frame.diagnostics;
            std::printf("Frame %lld:\n", (long long)frame.frame_number);
            std::printf("  cache: property_hits=%zu, misses=%zu\n", d.property_hits, d.property_misses);
            std::printf("         layer_hits=%zu, misses=%zu\n", d.layer_hits, d.layer_misses);
            std::printf("         composition_hits=%zu, misses=%zu\n", d.composition_hits, d.composition_misses);
            std::printf("  evaluated: properties=%zu, layers=%zu, compositions=%zu\n", 
                d.properties_evaluated, d.layers_evaluated, d.compositions_evaluated);
            if (!d.composition_key_manifest.empty()) std::printf("  composition_key: %s\n", d.composition_key_manifest.c_str());
            if (!d.frame_key_manifest.empty()) std::printf("  frame_key: %s\n", d.frame_key_manifest.c_str());
        }

        result.frame_diagnostics.push_back(frame.diagnostics);
        result.frames.push_back(std::move(frame));
        if (!result.output_error.empty()) {
            break;
        }
    }
    
    // Tier 1 Aggregated Session Summary
    const char* diag_env = std::getenv("TACHYON_DIAGNOSTICS");
    if (diag_env && std::string_view(diag_env) == "1" && !result.frame_diagnostics.empty()) {
        std::size_t total_prop_hits = 0;
        std::size_t total_prop_misses = 0;
        std::size_t total_layer_hits = 0;
        std::size_t total_layer_misses = 0;
        std::size_t total_comp_hits = 0;
        std::size_t total_comp_misses = 0;

        for (const auto& d : result.frame_diagnostics) {
            total_prop_hits += d.property_hits;
            total_prop_misses += d.property_misses;
            total_layer_hits += d.layer_hits;
            total_layer_misses += d.layer_misses;
            total_comp_hits += d.composition_hits;
            total_comp_misses += d.composition_misses;
        }

        std::printf("\n--- Session Diagnostic Summary ---\n");
        const auto print_stat = [](const char* name, std::size_t hits, std::size_t misses) {
            const std::size_t total = hits + misses;
            const double ratio = (total > 0) ? (100.0 * hits / total) : 0.0;
            std::printf("  %s Cache: %zu hits, %zu misses (%.1f%% hit rate)\n", name, hits, misses, ratio);
        };
        print_stat("Property", total_prop_hits, total_prop_misses);
        print_stat("Layer", total_layer_hits, total_layer_misses);
        print_stat("Composition", total_comp_hits, total_comp_misses);
        std::printf("----------------------------------\n\n");
    }

    if (sink && result.output_error.empty() && !sink->finish()) {
        result.output_error = sink->last_error();
    }

    result.diagnostics = context.media->consume_diagnostics();
    return result;
}

} // namespace tachyon
