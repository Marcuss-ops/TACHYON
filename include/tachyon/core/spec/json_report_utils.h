#pragma once

#include <nlohmann/json.hpp>
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"

namespace tachyon::spec {

using json = nlohmann::json;

json frame_range_to_json(const FrameRange& r);
json output_destination_to_json(const OutputDestination& d);
json output_profile_to_json(const OutputProfile& p);
json diagnostics_to_json(const DiagnosticBag& bag);
std::string diagnostics_to_text(const DiagnosticBag& bag);

} // namespace tachyon::spec
