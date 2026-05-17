#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/core/spec/validation/scene_validator.h"
#include "tachyon/runtime/core/data/compiled_scene.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/resource/render_context.h"
#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/transition_registry.h"
#include "tachyon/text/fonts/core/font_registry.h"
#include <filesystem>
#include <iostream>
#include <fstream>

namespace tachyon {

namespace {

struct PreflightResult {
    bool success{true};
    std::string error_message;
};

PreflightResult perform_preflight(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    const RenderContext& context) {
    
    // 1. Scene Validation
    core::SceneValidator validator;
    auto val_result = validator.validate(scene);
    if (!val_result.is_valid()) {
        return {false, "Scene validation failed: " + val_result.issues[0].message};
    }

    // 2. Asset Check
    for (const auto& asset : compiled_scene.assets) {
        if (!asset.path.empty() && !std::filesystem::exists(asset.path)) {
            return {false, "Asset not found: " + asset.path};
        }
    }

    // 3. Font Check
#ifdef TACHYON_ENABLE_TEXT
    if (context.font_registry) {
        for (const auto& comp : compiled_scene.compositions) {
            for (const auto& layer : comp.layers) {
                if (!layer.text.font_id.empty() && !context.font_registry->find(layer.text.font_id)) {
                     return {false, "Font not found: " + layer.text.font_id};
                }
            }
        }
    }
#endif

    // 4. Effect & Transition Check
    if (context.effects && context.transition_registry) {
        for (const auto& comp : compiled_scene.compositions) {
            for (const auto& layer : comp.layers) {
                for (const auto& effect : layer.effects) {
                    if (!context.effects->registry().find(effect.type)) {
                        return {false, "Unknown effect: " + effect.type};
                    }
                }
                if (!layer.transition_in.transition_id.empty() && layer.transition_in.transition_id != "none") {
                    if (!context.transition_registry->resolve(layer.transition_in.transition_id)) {
                        return {false, "Unknown transition: " + layer.transition_in.transition_id};
                    }
                }
                if (!layer.transition_out.transition_id.empty() && layer.transition_out.transition_id != "none") {
                    if (!context.transition_registry->resolve(layer.transition_out.transition_id)) {
                        return {false, "Unknown transition: " + layer.transition_out.transition_id};
                    }
                }
            }
        }
    }

    // 5. Output Writability
    std::filesystem::path out_path = execution_plan.render_plan.output.destination.path;
    if (!out_path.empty()) {
        auto parent = out_path.parent_path();
        if (!parent.empty() && !std::filesystem::exists(parent)) {
            std::error_code ec;
            if (!std::filesystem::create_directories(parent, ec)) {
                 return {false, "Could not create output directory: " + parent.string()};
            }
        }
        // Try to open file for writing (minimal check)
        std::ofstream ofs(out_path, std::ios::app);
        if (!ofs) {
            return {false, "Output path not writable: " + out_path.string()};
        }
    }

    return {true, ""};
}

} // namespace

bool RenderSession::preflight(
    const SceneSpec& scene,
    const CompiledScene& compiled_scene,
    const RenderExecutionPlan& execution_plan,
    const RenderContext& context,
    RenderSessionResult& result) {
    
    auto res = perform_preflight(scene, compiled_scene, execution_plan, context);
    if (!res.success) {
        result.output_error = "Preflight failed: " + res.error_message;
        std::cerr << "[Preflight] Error: " << res.error_message << std::endl;
        return false;
    }
    return true;
}

} // namespace tachyon
