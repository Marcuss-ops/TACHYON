#pragma once

#include "tachyon/core/spec/schema/contracts/shared_contracts.h"

#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace tachyon {

using CameraCut = spec::CameraCut;

/**
 * @brief Resolves the active camera for a given composition time.
 * 
 * Cuts are stored in start-time order. The resolver finds the cut that
 * contains the query time. If no cut contains the time, the default camera
 * (first cut or explicit default) is used.
 * 
 * Validation rules:
 * - Cuts must not overlap (hard cuts only).
 * - Cuts must reference existing camera layer IDs.
 * - Gaps between cuts use the previous cut's camera (hold).
 */
class CameraCutTimeline {
public:
    /**
     * @brief Add a camera cut. Must be added in start-time order.
     * 
     * The caller is responsible for ordering. Overlapping cuts are an error.
     */
    void add_cut(const CameraCut& cut);

    /**
     * @brief Get the active camera ID at the given time.
     * 
     * @return The camera ID, or nullopt if no cuts have been added.
     */
    [[nodiscard]] std::optional<std::string> active_camera_at(double time) const;

    /**
     * @brief Validate the cut list.
     * 
     * Checks:
     * - No overlapping cuts.
     * - All camera IDs are non-empty.
     * - Cuts are in start-time order.
     * 
     * Returns true if valid. If invalid, fills out_error.
     */
    [[nodiscard]] bool validate(std::string& out_error) const;

    [[nodiscard]] const std::vector<CameraCut>& cuts() const noexcept { return cuts_; }
    void set_cuts(std::vector<CameraCut> cuts) { cuts_ = std::move(cuts); }
    void clear() { cuts_.clear(); }
    bool empty() const { return cuts_.empty(); }

private:
    std::vector<CameraCut> cuts_;
};

} // namespace tachyon
