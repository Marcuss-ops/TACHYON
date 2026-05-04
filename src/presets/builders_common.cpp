#include "tachyon/presets/builders_common.h"
#include "tachyon/presets/transition/transition_builders.h"

namespace tachyon::presets {

LayerSpec make_base_layer(
    const std::string& id,
    const std::string& name,
    const std::string& type,
    const LayerParams& p) {
    
    LayerSpec l;
    l.id          = id;
    l.name        = name;
    l.type        = type;
    l.enabled     = true;
    l.visible     = true;
    l.start_time  = p.in_point;
    l.in_point    = p.in_point;
    l.out_point   = p.out_point;
    l.width       = static_cast<int>(p.w);
    l.height      = static_cast<int>(p.h);
    l.opacity     = static_cast<double>(p.opacity);
    
    l.transform.position_x = static_cast<double>(p.x);
    l.transform.position_y = static_cast<double>(p.y);
    l.transform.scale_x    = 1.0;
    l.transform.scale_y    = 1.0;
    l.transform.rotation   = 0.0;

    return l;
}

void apply_layer_transitions(
    LayerSpec& layer,
    const std::string& enter_id,
    double enter_duration,
    const std::string& exit_id,
    double exit_duration) {
    
    if (!enter_id.empty()) {
        TransitionParams tp;
        tp.id = enter_id;
        tp.duration = enter_duration;
        layer.transition_in = build_transition_enter(tp);
    }
    
    if (!exit_id.empty()) {
        TransitionParams tp;
        tp.id = exit_id;
        tp.duration = exit_duration;
        layer.transition_out = build_transition_exit(tp);
    }
}

} // namespace tachyon::presets
