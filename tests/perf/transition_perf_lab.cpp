#include "tachyon/media/management/media_manager.h"
#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/transition_registry.h"
#include "tachyon/renderer2d/effects/core/transitions/basic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/artistic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/light_leak_transitions.h"
#include "tachyon/output/frame_output_sink.h"
#include "tachyon/renderer2d/surface/surface_sampling.h"
#include <iostream>
#include <chrono>
#include <filesystem>
#include <algorithm>

using namespace tachyon;

renderer2d::SurfaceRGBA resize_surface_lab(const renderer2d::SurfaceRGBA& src, std::uint32_t width, std::uint32_t height) {
    if (src.width() == width && src.height() == height) return src;
    renderer2d::SurfaceRGBA out(width, height);
    out.set_profile(src.profile());
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(width);
            float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(height);
            out.set_pixel(x, y, renderer2d::sample_texture_bilinear(src, u, v, renderer2d::Color::white()));
        }
    }
    return out;
}

int main() {
    std::cout << "--- Tachyon Transition Lab ---" << std::endl;
    
    media::MediaManager media_manager;
    std::filesystem::path clip_a_path = "output/transitions/clip_a.mp4";
    std::filesystem::path clip_b_path = "output/transitions/clip_b.mp4";
    
    std::cout << "Loading clips..." << std::endl;
    auto surface_a_ptr = media_manager.get_video_frame(clip_a_path, 0.5);
    auto surface_b_ptr = media_manager.get_video_frame(clip_b_path, 0.5);
    
    if (!surface_a_ptr || !surface_b_ptr) {
        std::cerr << "Failed to load clips from " << clip_a_path << " or " << clip_b_path << std::endl;
        return 1;
    }
    
    renderer2d::SurfaceRGBA surface_a = *surface_a_ptr;
    renderer2d::SurfaceRGBA surface_b = resize_surface_lab(*surface_b_ptr, surface_a.width(), surface_a.height());
    
    std::cout << "Resolution: " << surface_a.width() << "x" << surface_a.height() << std::endl;

    TransitionRegistry transition_registry;
    register_builtin_transitions(transition_registry);
    for (auto* desc : transition_registry.list_all()) {
        if (!desc) continue;
        renderer2d::resolve_basic_transition_implementations(const_cast<TransitionDescriptor&>(*desc));
        renderer2d::resolve_artistic_transition_implementations(const_cast<TransitionDescriptor&>(*desc));
        renderer2d::resolve_light_leak_implementations(const_cast<TransitionDescriptor&>(*desc));
    }

    renderer2d::EffectRegistry effect_registry;
    renderer2d::register_builtin_effects(effect_registry, transition_registry);
    auto host = renderer2d::create_effect_host(effect_registry);

    std::vector<std::string> transitions_to_test;
    for (auto* desc : transition_registry.list_all()) {
        if (desc) {
            transitions_to_test.push_back(desc->id);
        }
    }
    
    // Sort for consistent output
    std::sort(transitions_to_test.begin(), transitions_to_test.end());

    std::filesystem::create_directories("output/transitions/clip_pair_demos");

    for (const auto& tid : transitions_to_test) {
        std::cout << "Benchmarking: " << tid << "..." << std::endl;
        
        RenderPlan plan;
        plan.composition.width = surface_a.width();
        plan.composition.height = surface_a.height();
        plan.composition.frame_rate = {30, 1};
        plan.composition.duration = 2.0;
        plan.output.destination.path = "output/transitions/clip_pair_demos/" + tid + ".mp4";
        plan.output.destination.overwrite = true;
        plan.output.profile.class_name = "video";
        plan.output.profile.container = "mp4";
        plan.output.profile.video.codec = "libx264";
        plan.output.profile.video.pixel_format = "yuv420p";
        
        auto sink = output::create_frame_output_sink(plan);
        if (!sink || !sink->begin(plan)) {
            std::cerr << "Failed to create sink for " << tid << std::endl;
            continue;
        }

        auto start_time = std::chrono::high_resolution_clock::now();
        
        const int frame_count = 10;
        const int transition_frames = 5;
        const int start_transition = 3; 
        
        for (int i = 0; i < frame_count; ++i) {
            float t = 0.0f;
            if (i < start_transition) {
                t = 0.0f;
            } else if (i < start_transition + transition_frames) {
                t = static_cast<float>(i - start_transition) / static_cast<float>(transition_frames - 1);
            } else {
                t = 1.0f;
            }
            
            renderer2d::EffectParams params;
            params.params["t"] = t;
            params.params["transition_id"] = std::string(tid);
            params.params["transition_to"] = static_cast<const renderer2d::SurfaceRGBA*>(&surface_b);

            auto result = host->apply("tachyon.effect.transition.glsl", surface_a, params);
            if (result.value.has_value()) {
                output::OutputFramePacket packet;
                packet.frame_number = i;
                packet.frame = &result.value.value();
                sink->write_frame(packet);
            }
        }
        
        sink->finish();
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        std::cout << "Done in " << duration << "ms (" << (static_cast<double>(duration) / frame_count) << " ms/frame)" << std::endl;
    }

    return 0;
}
