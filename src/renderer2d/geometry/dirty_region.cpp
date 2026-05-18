#include "tachyon/renderer2d/geometry/dirty_region.h"

namespace tachyon::renderer2d {

void DirtyRegion::add(IntRect rect) {
    if (!rect.empty()) {
        rects_.push_back(rect);
    }
}

void DirtyRegion::add_union(IntRect rect) {
    if (rect.empty()) return;

    if (rects_.empty()) {
        rects_.push_back(rect);
        return;
    }

    // Version that unifies into a single rectangle.
    // A future step could optimize this to merge overlapping rects while keeping independent ones separate.
    IntRect merged = rects_.front();
    for (const auto& r : rects_) {
        merged = merged.united(r);
    }
    merged = merged.united(rect);

    rects_.clear();
    rects_.push_back(merged);
}

void DirtyRegion::clear() {
    rects_.clear();
}

bool DirtyRegion::empty() const noexcept {
    return rects_.empty();
}

const std::vector<IntRect>& DirtyRegion::rects() const noexcept {
    return rects_;
}

IntRect DirtyRegion::bounds() const noexcept {
    IntRect result;
    for (const auto& r : rects_) {
        result = result.united(r);
    }
    return result;
}

void DirtyRegion::optimize() {
    if (rects_.size() <= 1) return;

    IntRect merged = rects_.front();
    for (const auto& r : rects_) {
        merged = merged.united(r);
    }

    rects_.clear();
    rects_.push_back(merged);
}

} // namespace tachyon::renderer2d