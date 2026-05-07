#include "tachyon/scene/builder.h"

namespace tachyon::scene {

// TransitionBuilder
TransitionBuilder& TransitionBuilder::id(std::string transition_id) {
    spec_.transition_id = std::move(transition_id);
    spec_.type = spec_.transition_id.empty() ? "none" : spec_.transition_id;
    return *this;
}

TransitionBuilder& TransitionBuilder::duration(double d) {
    spec_.duration = d;
    return *this;
}

TransitionBuilder& TransitionBuilder::ease(animation::EasingPreset e) {
    spec_.easing = e;
    return *this;
}

TransitionBuilder& TransitionBuilder::delay(double d) {
    spec_.delay = d;
    return *this;
}

LayerBuilder& TransitionBuilder::done() {
    return parent_;
}

} // namespace tachyon::scene
