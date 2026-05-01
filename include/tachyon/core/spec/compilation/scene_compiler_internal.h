#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/runtime/core/data/compiled_scene.h"
#include "tachyon/core/spec/compilation/compilation_context.h"
#include <unordered_map>
#include <cstdint>

namespace tachyon {

template<typename T>
CompiledPropertyTrack compile_property_track(
    CompilationRegistry& registry,
    const std::string& id_suffix,
    const std::string& layer_id,
    const T& property_spec,
    double fallback_value = 0.0);

// Explicit instantiations
extern template CompiledPropertyTrack compile_property_track<AnimatedScalarSpec>(
    CompilationRegistry&, const std::string&, const std::string&, const AnimatedScalarSpec&, double);

extern template CompiledPropertyTrack compile_property_track<AnimatedVector2Spec>(
    CompilationRegistry&, const std::string&, const std::string&, const AnimatedVector2Spec&, double);

extern template CompiledPropertyTrack compile_property_track<AnimatedVector3Spec>(
    CompilationRegistry&, const std::string&, const std::string&, const AnimatedVector3Spec&, double);

} // namespace tachyon
