#include "tachyon/tracker/planar_track_binding.h"
#include <cassert>

namespace tachyon::tracker {

PlanarTrackBinding::PlanarTrackBinding(const PlanarTrack& track,
                                         float layer_width,
                                         float layer_height)
    : m_track(track),
      m_layer_width(layer_width),
      m_layer_height(layer_height) {}

void PlanarTrackBinding::apply(int frame_number,
                               scene::EvaluatedLayerState& layer) const {
    const auto* frame = m_track.frame_at(frame_number);
    if (!frame || !frame->valid) {
        return; // No valid track data: leave layer unchanged
    }

    // Ensure corner_pin has exactly 4 entries
    layer.corner_pin.resize(4);

    // Map tracked corners to layer corner-pin.
    // The PlanarTrack corners are in screen-space pixel coordinates.
    // We write them directly into the layer state.
    for (int i = 0; i < 4; ++i) {
        layer.corner_pin[i] = math::Vector2{
            frame->corners[i].x,
            frame->corners[i].y
        };
    }
}

} // namespace tachyon::tracker
