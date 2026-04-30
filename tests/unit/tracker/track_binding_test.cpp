#include <gtest/gtest.h>
#include "tachyon/tracker/track.h"
#include "tachyon/tracker/track_binding.h"
#include "tachyon/core/scene/state/evaluated_state.h"
#include <cmath>

namespace tachyon::tracker {

class TrackBindingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple track with 2 samples
        track.name = "TestTrack";
        
        TrackSample s1;
        s1.time = 0.0f;
        s1.position = {100.0f, 200.0f};
        s1.confidence = 1.0f;
        track.samples.push_back(s1);
        
        TrackSample s2;
        s2.time = 1.0f;
        s2.position = {150.0f, 250.0f};
        s2.confidence = 1.0f;
        track.samples.push_back(s2);
    }
    
    Track track;
};

TEST_F(TrackBindingTest, PositionBindingInterpolates) {
    TrackBinding binding(track, TrackBindingTarget::Position);
    
    scene::EvaluatedLayerState layer;
    layer.local_transform.translate = {0.0f, 0.0f};
    
    // At t=0.5, should interpolate to (125, 225)
    binding.apply(0.5f, layer);
    
    EXPECT_NEAR(layer.local_transform.translate.x, 125.0f, 0.1f);
    EXPECT_NEAR(layer.local_transform.translate.y, 225.0f, 0.1f);
}

TEST_F(TrackBindingTest, PositionBindingAtExactTime) {
    TrackBinding binding(track, TrackBindingTarget::Position);
    
    scene::EvaluatedLayerState layer;
    
    binding.apply(0.0f, layer);
    EXPECT_FLOAT_EQ(layer.local_transform.translate.x, 100.0f);
    EXPECT_FLOAT_EQ(layer.local_transform.translate.y, 200.0f);
    
    binding.apply(1.0f, layer);
    EXPECT_FLOAT_EQ(layer.local_transform.translate.x, 150.0f);
    EXPECT_FLOAT_EQ(layer.local_transform.translate.y, 250.0f);
}

TEST_F(TrackBindingTest, PositionBindingBeforeFirstSample) {
    TrackBinding binding(track, TrackBindingTarget::Position);
    
    scene::EvaluatedLayerState layer;
    layer.local_transform.translate = {999.0f, 999.0f};
    
    // Time before first sample should clamp to first
    binding.apply(-1.0f, layer);
    EXPECT_FLOAT_EQ(layer.local_transform.translate.x, 100.0f);
    EXPECT_FLOAT_EQ(layer.local_transform.translate.y, 200.0f);
}

TEST_F(TrackBindingTest, PositionBindingAfterLastSample) {
    TrackBinding binding(track, TrackBindingTarget::Position);
    
    scene::EvaluatedLayerState layer;
    
    // Time after last sample should clamp to last
    binding.apply(10.0f, layer);
    EXPECT_FLOAT_EQ(layer.local_transform.translate.x, 150.0f);
    EXPECT_FLOAT_EQ(layer.local_transform.translate.y, 250.0f);
}

TEST_F(TrackBindingTest, RotationBindingWithAffine) {
    // Create track with affine data
    Track affine_track;
    affine_track.name = "AffineTrack";
    
    TrackSample s;
    s.time = 0.0f;
    s.position = {0.0f, 0.0f};
    affine_track.samples.push_back(s);
    
    // Add affine: 30 degree rotation (approx 0.5236 rad)
    std::vector<std::vector<float>> affines = {{0.866f, 0.5f, 0.0f, -0.5f, 0.866f, 0.0f}};
    affine_track.affine_per_sample = affines;
    
    TrackBinding binding(affine_track, TrackBindingTarget::Rotation);
    
    scene::EvaluatedLayerState layer;
    layer.local_transform.rotation = 0.0f;
    
    binding.apply(0.0f, layer);
    
    // Should extract rotation from affine matrix
    EXPECT_NEAR(layer.local_transform.rotation, 30.0f, 1.0f); // 30 degrees
}

TEST_F(TrackBindingTest, ScaleBindingWithAffine) {
    Track affine_track;
    affine_track.name = "ScaleTrack";
    
    TrackSample s;
    s.time = 0.0f;
    s.position = {0.0f, 0.0f};
    affine_track.samples.push_back(s);
    
    // Add affine: scale x2 (uniform)
    std::vector<std::vector<float>> affines = {{2.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f}};
    affine_track.affine_per_sample = affines;
    
    TrackBinding binding(affine_track, TrackBindingTarget::Scale);
    
    scene::EvaluatedLayerState layer;
    layer.local_transform.scale = {1.0f, 1.0f};
    
    binding.apply(0.0f, layer);
    
    EXPECT_FLOAT_EQ(layer.local_transform.scale.x, 2.0f);
    EXPECT_FLOAT_EQ(layer.local_transform.scale.y, 2.0f);
}

TEST_F(TrackBindingTest, SilentlyFailsOnMissingData) {
    Track empty_track;
    empty_track.name = "EmptyTrack";
    
    TrackBinding binding(empty_track, TrackBindingTarget::Position);
    
    scene::EvaluatedLayerState layer;
    layer.local_transform.translate = {50.0f, 50.0f};
    
    // Should not crash, should leave layer unchanged
    binding.apply(0.0f, layer);
    
    EXPECT_FLOAT_EQ(layer.local_transform.translate.x, 50.0f);
    EXPECT_FLOAT_EQ(layer.local_transform.translate.y, 50.0f);
}

TEST_F(TrackBindingTest, TrackBindingProperties) {
    TrackBinding binding(track, TrackBindingTarget::Position);
    
    EXPECT_EQ(binding.target(), TrackBindingTarget::Position);
    EXPECT_FLOAT_EQ(binding.track().samples[0].time, 0.0f);
}

} // namespace tachyon::tracker
