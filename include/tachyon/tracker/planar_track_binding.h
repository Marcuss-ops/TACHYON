#pragma once

#include "tachyon/tracker/planar_track.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include <string>

namespace tachyon::tracker {

/**
 * @brief Binding that drives a layer's corner-pin from a PlanarTrack.
 *
 * A PlanarTrack represents a rigid planar surface tracked over time.
 * This binding maps the four corners of a layer to the four tracked
 * corners of the planar surface, producing a corner-pin / surface-insert
 * effect (Mocha-style).
 *
 * The binding is explicit and deterministic:
 * - It reads the PlanarTrack at the given frame number.
 * - It writes the four corner positions into the layer's corner_pin array.
 * - If the track is invalid at that frame, the layer is left unchanged.
 */
class PlanarTrackBinding {
public:
    /**
     * @param track        The PlanarTrack to read from (must outlive the binding).
     * @param layer_width  Width of the layer content in pixels.
     * @param layer_height Height of the layer content in pixels.
     */
    PlanarTrackBinding(const PlanarTrack& track,
                       float layer_width,
                       float layer_height);

    /**
     * @brief Apply the planar track at the given frame to the layer state.
     *
     * Writes the four tracked corners into layer.corner_pin[4].
     * The corners are in screen-space pixel coordinates.
     * If the track has no valid data at this frame, the layer is unchanged.
     */
    void apply(int frame_number, scene::EvaluatedLayerState& layer) const;

    [[nodiscard]] const PlanarTrack& track() const noexcept { return m_track; }
    [[nodiscard]] float layer_width() const noexcept { return m_layer_width; }
    [[nodiscard]] float layer_height() const noexcept { return m_layer_height; }

private:
    const PlanarTrack& m_track;
    float m_layer_width;
    float m_layer_height;
};

} // namespace tachyon::tracker
