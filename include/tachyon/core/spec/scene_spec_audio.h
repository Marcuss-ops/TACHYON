#pragma once

#include "tachyon/core/spec/schema/audio/audio_spec.h"
#include "tachyon/core/spec/schema/objects/composition_spec.h"

#include <nlohmann/json.hpp>

namespace tachyon {

class DiagnosticBag;

void parse_composition_audio_tracks(CompositionSpec& composition,
                                     const nlohmann::json& object,
                                     const std::string& path,
                                     DiagnosticBag& diagnostics);

} // namespace tachyon
