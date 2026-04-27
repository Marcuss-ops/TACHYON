#include "tachyon/runtime/execution/frames/frame_executor.h"
#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/renderer2d/resource/render_context.h"
#include "tachyon/runtime/cache/frame_cache.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/text/fonts/font_registry.h"
#include "cli_internal.h"
#include <iostream>

namespace tachyon {

bool run_preview_frame_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    SceneSpec scene;
    AssetResolutionTable assets;
    if (!load_scene_context(options.scene_path, scene, assets, err)) return false;

    const auto job_parsed = parse_render_job_file(options.job_path);
    if (!job_parsed.value.has_value()) { 
        print_diagnostics(job_parsed.diagnostics, err); 
        return false; 
    }
    RenderJob job = *job_parsed.value;

    const auto plan_result = build_render_plan(scene, job);
    if (!plan_result.value.has_value()) { 
        print_diagnostics(plan_result.diagnostics, err); 
        return false; 
    }

    SceneCompiler compiler;
    const auto compiled_result = compiler.compile(scene);
    if (!compiled_result.ok()) { 
        print_diagnostics(compiled_result.diagnostics, err); 
        return false; 
    }

    text::FontRegistry font_registry;
    bool font_loaded = font_registry.load_ttf("main_font", "C:/Users/pater/Pyt/Tachyon/assets/fonts/SFPRODISPLAYBOLD.OTF", 128);
    out << "Font 'main_font' loaded: " << (font_loaded ? "YES" : "NO") << "\n";
    if (font_loaded) {
        const auto* font = font_registry.find("main_font");
        out << "Font found in registry: " << (font ? "YES" : "NO") << "\n";
    }
    font_registry.set_default("main_font");
    if (scene.font_manifest.has_value()) {
        for (const auto& font_entry : scene.font_manifest->fonts) {
            std::filesystem::path font_path = font_entry.src;
            if (font_path.is_relative()) {
                font_path = scene_asset_root(options.scene_path) / font_path;
            }
            // Load with a reasonable base size, Tachyon will scale as needed
            if (!font_registry.load_ttf(font_entry.id, font_path, 64)) {
                err << "Warning: Failed to load font: " << font_entry.id << " from " << font_path << "\n";
            }
        }
        if (!scene.font_manifest->fonts.empty()) {
            font_registry.set_default(scene.font_manifest->fonts.front().id);
        }
    }

    FrameArena arena;
    FrameCache cache;
    FrameExecutor executor(arena, cache);
    
    RenderContext context;
    context.policy = plan_result.value->quality_policy;
    context.renderer2d.policy = plan_result.value->quality_policy;
    context.renderer2d.font_registry = &font_registry;
    context.renderer2d.width = scene.compositions.empty() ? 1920 : scene.compositions.front().width;
    context.renderer2d.height = scene.compositions.empty() ? 1080 : scene.compositions.front().height;
    
    // Debug: Inspect compiled layers
    out << "Compiled scene has " << compiled_result.value->compositions.size() << " compositions.\n";
    if (!compiled_result.value->compositions.empty()) {
        const auto& comp = compiled_result.value->compositions.front();
        out << "Main composition has " << comp.layers.size() << " layers.\n";
        for (const auto& layer : comp.layers) {
            out << "Layer: " << layer.node.node_id << " (Topo: " << layer.node.topo_index << ") Type: " << layer.type_id << " Pos indices count: " << layer.property_indices.size() << "\n";
            out << "  Bounds: " << layer.in_time << " to " << layer.out_time << " (Start: " << layer.start_time << ")\n";
            for (auto idx : layer.property_indices) {
                if (idx < compiled_result.value->property_tracks.size()) {
                    const auto& track = compiled_result.value->property_tracks[idx];
                    out << "  Property: " << track.property_id << " Value: " << track.constant_value << "\n";
                }
            }
        }
    }

    FrameRenderTask task;
    task.frame_number = static_cast<std::int64_t>(*options.preview_frame_number);
    task.time_seconds = static_cast<double>(task.frame_number) / (scene.compositions.empty() ? 60.0 : scene.compositions.front().frame_rate.value());

    out << "Rendering frame " << task.frame_number << " (t=" << task.time_seconds << "s)...\n";
    
    ExecutedFrame result = executor.execute(*compiled_result.value, *plan_result.value, task, context);

    if (result.frame) {
        if (result.frame->save_png(options.preview_output)) {
            out << "Successfully saved preview to: " << options.preview_output << "\n";
            return true;
        } else {
            err << "Failed to save PNG to: " << options.preview_output << "\n";
            return false;
        }
    } else {
        err << "Render failed to produce a frame.\n";
        return false;
    }
}

} // namespace tachyon
