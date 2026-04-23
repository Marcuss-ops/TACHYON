#pragma once

#include "tachyon/tracker/track.h"
#include "tachyon/tracker/feature_tracker.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include <vector>
#include <optional>

namespace tachyon::tracker {

// High-level workflow wrapper for point tracking.
//
// Usage:
//   1. Create PointTracker with config
//   2. Call track_points() or track_with_affine() to get a Track
//   3. Bind the Track to layer properties via TrackBinding
class PointTracker {
public:
    explicit PointTracker(FeatureTracker::Config cfg = {}) : m_cfg(cfg) {}
    
    // Track points through a sequence of frames.
    // Returns a Track with position samples per frame.
    // Uses only the first detected feature for simplicity.
    static Track track_points(
        const std::vector<GrayImage>& frames,
        const FeatureTracker::Config& cfg = {});
    
    // Track points and solve affine transform per frame (requires >= 3 points).
    // Returns a Track with affine_per_sample filled.
    static Track track_with_affine(
        const std::vector<GrayImage>& frames,
        const FeatureTracker::Config& cfg = {});
    
    // Track points and solve homography per frame (requires >= 4 points).
    // Returns a Track with homography_per_sample filled.
    static Track track_with_homography(
        const std::vector<GrayImage>& frames,
        const FeatureTracker::Config& cfg = {});
    
    // Convenience: create a TrackBinding for a layer
    static TrackBinding create_binding(
        const Track& track,
        TrackBindingTarget target,
        const std::string& layer_id);
    
private:
    FeatureTracker::Config m_cfg;
};

// Layer property binding orchestrator.
// Manages multiple track bindings and applies them during scene evaluation.
class TrackBindingOrchestrator {
public:
    void add_binding(const TrackBinding& binding);
    void remove_binding(const std::string& track_id, const std::string& layer_id);
    
    // Apply all bindings for a given layer at time t
    void apply_to_layer(
        const std::string& layer_id,
        float time,
        ::tachyon::scene::EvaluatedLayerState& layer) const;
    
    // Apply all bindings for all layers (call during scene evaluation)
    void apply_all(
        const std::vector<::tachyon::scene::EvaluatedLayerState>& layers,
        float time) const;
    
    const std::vector<TrackBinding>& bindings() const { return m_bindings; }
    
private:
    std::vector<TrackBinding> m_bindings;
};

} // namespace tachyon::tracker
