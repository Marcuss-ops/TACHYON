#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/types/diagnostics.h"
#include <string>

namespace tachyon::pipeline {

/**
 * @brief Unified pipeline for Scene Specification operations.
 * Use void* for compiled types to avoid circular dependencies and syntax errors in authoring headers.
 */
class UnifiedSceneSpecPipeline {
public:
    static void CompileLayerProperties(
        const LayerSpec& layer, 
        void* compiled_layer_ptr, 
        void* compiled_scene_ptr,
        void* registry_ptr,
        DiagnosticBag& diagnostics);
};

} // namespace tachyon::pipeline
