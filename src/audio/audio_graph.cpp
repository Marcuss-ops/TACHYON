// audio_graph.cpp
// AudioGraph and AudioBus are fully inline/header-only.
// This translation unit exists to anchor the library into the build and
// to host any future non-trivial AudioGraph methods.

#include "tachyon/audio/audio_graph.h"
#include "tachyon/audio/dsp_nodes.h"

namespace tachyon::audio {

// Nothing to define: all AudioGraph / AudioBus / AudioNode methods
// are implemented inline in audio_graph.h and dsp_nodes.h.
//
// Future additions (e.g. sidechain routing, send busses) will be implemented here.

} // namespace tachyon::audio
