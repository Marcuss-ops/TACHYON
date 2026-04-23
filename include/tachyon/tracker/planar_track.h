#pragma once

#include "tachyon/tracker/feature_tracker.h"
#include <vector>
#include <string>

namespace tachyon::tracker {

// -------------------------------------------------------------------
// PlanarTrack: a tracked planar surface over time.
//
// Represents a rigid planar region (e.g. a wall, screen, signboard)
// tracked through a video sequence.  The four corners move in 2D and
// define a homography per frame that can drive corner-pin transforms,
// surface replacement, or matte placement.
// -------------------------------------------------------------------
struct PlanarTrack {

    // Per-frame state of the four tracked corners.
    // Corners are ordered: top-left, top-right, bottom-right, bottom-left
    // in the *initial* frame coordinate system.
    struct Frame {
        int frame_number{0};
        Point2f corners[4];
        // 3x3 homography that maps initial corners -> current corners
        // Row-major: H[0..8] = {h00,h01,h02,h10,h11,h12,h20,h21,h22}
        float homography[9]{1,0,0, 0,1,0, 0,0,1};
        float confidence{1.0f}; // 0 = lost, 1 = high confidence
        bool valid{false};
    };

    std::string name;           // user-defined track name
    std::vector<Frame> frames;  // one entry per tracked frame

    // Initial corner positions in the reference frame.
    Point2f initial_corners[4];
    int reference_frame{0};

    // Track metadata
    int width{0};   // source frame width (for normalisation)
    int height{0};  // source frame height

    // -----------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------

    // Return the frame entry for a given frame number, or nullptr.
    const Frame* frame_at(int frame_number) const {
        for (const auto& f : frames)
            if (f.frame_number == frame_number) return &f;
        return nullptr;
    }

    // Return the homography for a frame, or identity if missing.
    std::vector<float> homography_at(int frame_number) const {
        auto* f = frame_at(frame_number);
        if (f && f->valid)
            return std::vector<float>(f->homography, f->homography + 9);
        return {1,0,0, 0,1,0, 0,0,1};
    }

    // Return true if the track has valid data for the given frame.
    bool is_valid_at(int frame_number) const {
        auto* f = frame_at(frame_number);
        return f && f->valid;
    }
};

} // namespace tachyon::tracker
