#include "cli_internal.h"
#include "tachyon/runtime/execution/jobs/frame_executor.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/output/png_video_sink.h"
#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/core/scene/evaluator.h"
#include <fstream>

namespace tachyon {

bool run_preview_frame_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    // Carica scena e job (riusa cli_render.cpp)
    SceneSpec scene;
    AssetResolutionTable assets;
    
    if (!load_scene_context(options.scene_path, scene, assets, err)) {
        err << "Failed to load scene: " << options.scene_path << '\n';
        return false;
    }
    
    // Carica il render job
    RenderJobSpec job;
    if (!options.job_path.empty()) {
        auto job_result = RenderJobSpec::from_json_file(options.job_path);
        if (!job_result) {
            err << "Failed to load job: " << options.job_path << '\n';
            return false;
        }
        job = *job_result;
    } else {
        // Job default se non specificato
        job.composition_id = scene.compositions.empty() ? "" : scene.compositions[0].id;
        job.start_frame = 0;
        job.end_frame = 0;
        job.fps = 30.0;
        job.output_path = options.output_override;
    }
    
    const int frame_number = *options.preview_frame_number;
    
    // Trova la composizione
    const CompositionSpec* comp = nullptr;
    for (const auto& c : scene.compositions) {
        if (c.id == job.composition_id || comp == nullptr) {
            comp = &c;
            break;
        }
    }
    
    if (!comp) {
        err << "Composition not found: " << job.composition_id << '\n';
        return false;
    }
    
    // Crea un FrameExecutor per il singolo frame
    FrameExecutor executor(scene, job, assets);
    
    // Esegui solo il frame richiesto
    const double time_seconds = static_cast<double>(frame_number) / job.fps;
    auto frame_result = executor.execute_frame(frame_number, time_seconds);
    
    if (!frame_result) {
        err << "Failed to execute frame " << frame_number << '\n';
        return false;
    }
    
    // Scrivi PNG tramite il PNG sink esistente
    output::PngVideoSink png_sink;
    if (!png_sink.open(options.preview_output, comp->width, comp->height)) {
        err << "Failed to open output PNG: " << options.preview_output << '\n';
        return false;
    }
    
    // Converti framebuffer a RGBA e scrivi
    const auto& fb = frame_result->framebuffer;
    std::vector<uint8_t> rgba(comp->width * comp->height * 4);
    
    // Copia i dati dal framebuffer (assumendo formato float RGBA)
    for (std::size_t i = 0; i < rgba.size(); ++i) {
        rgba[i] = static_cast<uint8_t>(std::clamp(fb.data[i] * 255.0f, 0.0f, 255.0f));
    }
    
    if (!png_sink.write_frame(rgba.data(), comp->width * comp->height * 4)) {
        err << "Failed to write PNG frame\n";
        return false;
    }
    
    png_sink.close();
    
    out << "Preview frame " << frame_number << " written to " << options.preview_output << '\n';
    return true;
}

} // namespace tachyon
