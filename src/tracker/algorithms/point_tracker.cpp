#include "tachyon/tracker/algorithms/point_tracker.h"
#include "tachyon/tracker/algorithms/feature_tracker.h"
#include "tachyon/tracker/core/track.h"
#include "tachyon/tracker/core/track_binding.h"
#include <vector>
#include <algorithm>

namespace tachyon::tracker {

// High-level workflow: detect → track → return Track
Track PointTracker::track_points(
    const std::vector<GrayImage>& frames,
    const FeatureTracker::Config& cfg) {
    
    FeatureTracker tracker(cfg);
    Track track;
    track.name = "PointTrack";
    
    if (frames.empty()) return track;
    
    // Detect features in first frame
    auto points = tracker.detect_features(frames[0]);
    
    if (points.empty()) return track;
    
    // Track through all frames
    std::vector<Point2f> current_points = points;
    
    for (size_t i = 0; i < frames.size(); ++i) {
        TrackSample sample;
        sample.time = static_cast<float>(i);
        
        if (i == 0) {
            sample.position = points[0];
            sample.confidence = 1.0f;
        } else {
            auto tracked = tracker.track_frame(frames[i-1], frames[i], current_points);
            
            if (!tracked.empty() && tracked[0].confidence > cfg.min_confidence) {
                sample.position = tracked[0].position;
                sample.confidence = tracked[0].confidence;
                current_points.clear();
                for (const auto& t : tracked) current_points.push_back(t.position);
            } else {
                sample.position = current_points[0];
                sample.confidence = 0.0f;
            }
        }
        
        track.samples.push_back(sample);
    }
    
    return track;
}

// Track with affine solve (requires >= 3 points)
Track PointTracker::track_with_affine(
    const std::vector<GrayImage>& frames,
    const FeatureTracker::Config& cfg) {
    
    FeatureTracker tracker(cfg);
    Track track;
    track.name = "AffineTrack";
    
    if (frames.size() < 2) return track;
    
    auto points = tracker.detect_features(frames[0]);
    
    if (points.size() < 3) return track;
    
    for (size_t i = 0; i < frames.size(); ++i) {
        TrackSample sample;
        sample.time = static_cast<float>(i);
        sample.position = points[0];
        sample.confidence = 1.0f;
        track.samples.push_back(sample);
    }
    
    std::vector<Point2f> current_points = points;
    std::vector<std::vector<float>> affines;
    
    for (size_t i = 1; i < frames.size(); ++i) {
        auto tracked = tracker.track_frame(frames[i-1], frames[i], current_points);
        
        if (tracked.size() >= 3) {
            std::vector<TrackPoint> dst;
            for (const auto& t : tracked) dst.push_back({t.position, t.confidence, t.occluded});
            
            auto affine = FeatureTracker::estimate_affine(points, dst);
            
            if (affine.has_value()) {
                affines.push_back(*affine);
                track.samples[i].confidence = 1.0f;
            } else {
                affines.push_back({1,0,0, 0,1,0});
                track.samples[i].confidence = 0.0f;
            }
            
            current_points.clear();
            for (const auto& t : tracked) current_points.push_back(t.position);
        } else {
            affines.push_back({1,0,0, 0,1,0});
            track.samples[i].confidence = 0.0f;
        }
    }
    
    if (!affines.empty()) {
        track.affine_per_sample = affines;
    }
    
    return track;
}

// Track with homography solve (requires >= 4 points)
Track PointTracker::track_with_homography(
    const std::vector<GrayImage>& frames,
    const FeatureTracker::Config& cfg) {
    
    FeatureTracker tracker(cfg);
    Track track;
    track.name = "HomographyTrack";
    
    if (frames.size() < 2) return track;
    
    auto points = tracker.detect_features(frames[0]);
    
    if (points.size() < 4) return track;
    
    for (size_t i = 0; i < frames.size(); ++i) {
        TrackSample sample;
        sample.time = static_cast<float>(i);
        sample.position = points[0];
        sample.confidence = 1.0f;
        track.samples.push_back(sample);
    }
    
    std::vector<Point2f> current_points = points;
    std::vector<std::vector<float>> homographies;
    
    for (size_t i = 1; i < frames.size(); ++i) {
        auto tracked = tracker.track_frame(frames[i-1], frames[i], current_points);
        
        if (tracked.size() >= 4) {
            std::vector<TrackPoint> dst;
            for (const auto& t : tracked) dst.push_back({t.position, t.confidence, t.occluded});
            
            std::vector<bool> inlier_mask;
            auto homo = FeatureTracker::estimate_homography_ransac(
                points, dst, cfg.ransac_iterations, cfg.ransac_threshold, 
                cfg.ransac_min_inlier_ratio, inlier_mask);
            
            if (homo.has_value()) {
                homographies.push_back(*homo);
                track.samples[i].confidence = 1.0f;
            } else {
                homographies.push_back({1,0,0, 0,1,0, 0,0,1});
                track.samples[i].confidence = 0.0f;
            }
            
            current_points.clear();
            for (const auto& t : tracked) current_points.push_back(t.position);
        } else {
            homographies.push_back({1,0,0, 0,1,0, 0,0,1});
            track.samples[i].confidence = 0.0f;
        }
    }
    
    if (!homographies.empty()) {
        track.homography_per_sample = homographies;
    }
    
    return track;
}

// Convenience: create a TrackBinding for a layer
TrackBinding PointTracker::create_binding(
    const Track& track,
    TrackBindingTarget target) {
    return TrackBinding(track, target);
}

// --- TrackBindingOrchestrator Implementation ---

void TrackBindingOrchestrator::add_binding(const TrackBinding& binding) {
    m_bindings.push_back(binding);
}

void TrackBindingOrchestrator::remove_binding(
    const std::string& track_id, const std::string& /*layer_id*/) {
    m_bindings.erase(
        std::remove_if(m_bindings.begin(), m_bindings.end(),
            [&](const TrackBinding& b) {
                return b.track().name == track_id;
            }),
        m_bindings.end());
}

void TrackBindingOrchestrator::apply_to_layer(
    const std::string& /*layer_id*/,
    float time,
    ::tachyon::scene::EvaluatedLayerState& layer) const {
    for (const auto& binding : m_bindings) {
        // Apply all bindings for this layer
        binding.apply(time, layer);
    }
}

void TrackBindingOrchestrator::apply_all(
    const std::vector<::tachyon::scene::EvaluatedLayerState>& layers,
    float time) const {
    for (const auto& layer : layers) {
        apply_to_layer(/* layer.id */ "", time, const_cast<::tachyon::scene::EvaluatedLayerState&>(layer));
    }
}

} // namespace tachyon::tracker

