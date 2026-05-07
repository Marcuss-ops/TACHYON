#include "tachyon/runtime/execution/motion_blur_sampler.h"
#include "frame_executor_internal.h"
#include "tachyon/runtime/profiling/render_profiler.h"
#include "tachyon/renderer2d/raster/draw_list_builder.h"
#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/render/intent_builder.h"
#include "tachyon/renderer2d/resource/resource_provider.h"

#include <vector>
#include <memory>
#include <cmath>
#include <cstdlib>

#ifdef TACHYON_ENABLE_3D
#include "tachyon/renderer3d/core/ray_tracer.h"
#endif

#include <omp.h>
#include <thread>

namespace tachyon {

std::optional<std::shared_ptr<renderer2d::Framebuffer>> MotionBlurSampler::sample(
    FrameExecutor& executor,
    const CompiledScene& scene,
    const RenderPlan& plan,
    const FrameRenderTask& task,
    const DataSnapshot& snapshot,
    RenderContext& context,
    double frame_time_seconds,
    std::uint64_t composition_key,
    std::uint64_t frame_key,
    int fps,
    const CacheKeyBuilder& comp_builder
) {
    if (!plan.motion_blur_enabled || plan.motion_blur_samples <= 1) {
        return std::nullopt;
    }

    profiling::ProfileScope mb_scope(context.profiler, profiling::ProfileEventType::Phase, "motion_blur", task.frame_number);
    int samples = std::min<int>(static_cast<int>(plan.motion_blur_samples), plan.quality_policy.motion_blur_sample_cap);
    const double shutter_duration = (plan.motion_blur_shutter_angle / 360.0) / fps;
    const double shutter_start_offset = (plan.motion_blur_shutter_phase / 360.0) / fps;

    int omp_max_threads = 1;
    if (executor.parallel_frames()) {
        omp_max_threads = 1;
    } else {
        const char* omp_threads_env = std::getenv("TACHYON_OPENMP_THREADS_PER_FRAME");
        if (omp_threads_env) {
            char* endptr;
            long val = std::strtol(omp_threads_env, &endptr, 10);
            if (endptr != omp_threads_env && val > 0) {
                omp_max_threads = static_cast<int>(val);
            }
        } else {
            unsigned int hw = std::thread::hardware_concurrency();
            if (hw == 0) hw = 1;
            omp_max_threads = std::max(1u, hw / std::max(1u, static_cast<unsigned int>(executor.parallel_worker_count())));
        }

        const char* max_total_env = std::getenv("TACHYON_MAX_TOTAL_THREADS");
        if (max_total_env) {
            char* endptr;
            long max_total = std::strtol(max_total_env, &endptr, 10);
            if (endptr != max_total_env && max_total > 0) {
                const long total = static_cast<long>(executor.parallel_worker_count()) * omp_max_threads;
                if (total > max_total) {
                    omp_max_threads = std::max(1, static_cast<int>(max_total / executor.parallel_worker_count()));
                }
            }
        }
    }

    std::vector<std::shared_ptr<renderer2d::SurfaceRGBA>> samples_surfaces(samples);
#pragma omp parallel for num_threads(omp_max_threads)
    for (int s = 0; s < samples; ++s) {
        const double t_offset = shutter_start_offset + (samples > 1 ? (static_cast<double>(s) / (samples - 1)) * shutter_duration : 0.0);
        const double sub_frame_time = frame_time_seconds + t_offset;
        CacheKeyBuilder sample_builder = comp_builder;
        sample_builder.add_u64(static_cast<std::uint64_t>(task.frame_number));
        sample_builder.add_f64(sub_frame_time);
        const std::uint64_t sample_key = sample_builder.finish();
        const std::uint64_t root_sample_key = build_node_key(sample_key, scene.compositions.front().node);

        RenderContext thread_context = context;
        thread_context.renderer2d.accumulation_buffer.resize(0);
#ifdef TACHYON_ENABLE_3D
        thread_context.ray_tracer = std::make_shared<renderer3d::RayTracer>(thread_context.renderer2d.media_manager);
        thread_context.renderer2d.ray_tracer = thread_context.ray_tracer;
#endif

        const auto& topo_order = scene.graph.topo_order();
        for (std::uint32_t node_id : topo_order) {
            if (context.cancel_flag && context.cancel_flag->load()) break;
            ::tachyon::evaluate_node(executor, node_id, scene, plan, snapshot, thread_context, composition_key, sample_key, sub_frame_time, task, frame_key, frame_time_seconds);
        }
        if (executor.cache().lookup_composition(root_sample_key) == nullptr) {
            ::tachyon::evaluate_composition(executor, scene, scene.compositions.front(), plan, snapshot, thread_context, composition_key, root_sample_key, sample_key, sub_frame_time, task);
        }
        auto sub_comp = executor.cache().lookup_composition(root_sample_key);
        if (sub_comp) {
            renderer2d::RendererResourceProvider provider(thread_context.renderer2d);
            const auto intent_result = render::build_render_intent(*sub_comp, &provider);
            RasterizedFrame2D rasterized = render_evaluated_composition_2d(*sub_comp, intent_result.intent, plan, task, thread_context.renderer2d);
            if (rasterized.surface) samples_surfaces[s] = rasterized.surface;
        }
    }

    std::unique_ptr<renderer2d::Framebuffer> accumulated;
    for (int s = 0; s < samples; ++s) {
        if (samples_surfaces[s]) {
            if (!accumulated) accumulated = std::make_unique<renderer2d::Framebuffer>(std::move(*samples_surfaces[s]));
            else {
                auto& acc_pixels = accumulated->mutable_pixels();
                const auto& sub_pixels = samples_surfaces[s]->pixels();
                const std::size_t count = std::min(acc_pixels.size(), sub_pixels.size());
                for (std::size_t i = 0; i < count; ++i) acc_pixels[i] += sub_pixels[i];
            }
        }
    }
    if (accumulated) {
        auto& acc_pixels = accumulated->mutable_pixels();
        const float inv_samples = 1.0f / static_cast<float>(samples);
        for (float& p : acc_pixels) p *= inv_samples;
        return std::shared_ptr<renderer2d::Framebuffer>(std::move(accumulated));
    }
    return std::nullopt;
}

} // namespace tachyon
