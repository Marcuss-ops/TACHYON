#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include <nlohmann/json.hpp>
#include <string>

namespace tachyon::pipeline {

/**
 * @brief Unified pipeline for Scene Specification operations.
 * Use void* for compiled types to avoid circular dependencies and syntax errors in authoring headers.
 */
class UnifiedSceneSpecPipeline {
public:
    static void ParseLayer(const nlohmann::json& j, LayerSpec& out, const std::string& path, DiagnosticBag& diagnostics);
    static nlohmann::json SerializeLayer(const LayerSpec& layer);

    static void CompileLayerProperties(
        const LayerSpec& layer, 
        void* compiled_layer_ptr, 
        void* compiled_scene_ptr,
        void* registry_ptr,
        DiagnosticBag& diagnostics);
};

} // namespace tachyon::pipeline
