#pragma once

#include "tachyon/tracker/core/track.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include <string>

namespace tachyon::tracker {

/**
 * @brief Target property that a track can drive.
 * 
 * The binding is explicit: a Track + a TargetProperty + a Layer.
 * No implicit behavior. No global state.
 */
enum class TrackBindingTarget {
    Position,   ///< Drive layer 2D position from tracked point
    Rotation,   ///< Drive layer rotation from affine solve
    Scale,      ///< Drive layer scale from affine solve
    CornerPin   ///< Drive corner-pin from homography solve
};

/**
 * @brief Binds a Track to an EvaluatedLayerState property at evaluation time.
 * 
 * This is a deterministic transform modifier. It reads the track at the
 * given composition time and writes the result into the layer's local transform.
 * It does not own the track — the track lifetime must outlive the binding.
 */
class TrackBinding {
public:
    TrackBinding(const Track& track, TrackBindingTarget target);
    
    /**
     * @brief Apply the track at the given time to the layer state.
     * 
     * If the track has no sample at this time, the layer is left unchanged.
     * This is intentional: fail silently only when data is missing, never
     * when data is corrupt.
     */
    void apply(float time, scene::EvaluatedLayerState& layer) const;
    
    [[nodiscard]] const Track& track() const noexcept { return *m_track; }
    [[nodiscard]] TrackBindingTarget target() const noexcept { return m_target; }
    
private:
    const Track* m_track{nullptr};
    TrackBindingTarget m_target;
};

} // namespace tachyon::tracker

