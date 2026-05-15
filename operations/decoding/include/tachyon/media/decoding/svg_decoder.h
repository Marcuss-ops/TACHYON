#pragma once

#include "tachyon/core/media/decoding/svg_decoder.h"

namespace tachyon::media {

using ParsedSvg = ::tachyon::core::media::ParsedSvg;

bool parse_svg_string(const std::string& svg_content, ParsedSvg& out_result, DiagnosticBag& diagnostics);

} // namespace tachyon::media
