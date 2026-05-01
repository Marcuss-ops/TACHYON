#include "tachyon/core/spec/compilation/property_track_compiler.h"
#include "tachyon/core/spec/compilation/scene_compiler_internal.h"

namespace tachyon {

template CompiledPropertyTrack compile_property_track<AnimatedScalarSpec>(
    CompilationRegistry&, const std::string&, const std::string&, const AnimatedScalarSpec&, double);

template CompiledPropertyTrack compile_property_track<AnimatedVector2Spec>(
    CompilationRegistry&, const std::string&, const std::string&, const AnimatedVector2Spec&, double);

template CompiledPropertyTrack compile_property_track<AnimatedVector3Spec>(
    CompilationRegistry&, const std::string&, const std::string&, const AnimatedVector3Spec&, double);

} // namespace tachyon
