#include "tachyon/presets/builders_common.h"
#include "tachyon/presets/transition/transition_preset_registry.h"

namespace tachyon::presets {

LayerSpec make_base_layer(
    const std::string& id,
    const std::string& name,
    const std::string& type,
    const LayerParams& p) {
    
    LayerSpec l;
    l.identity.id      = id;
    l.identity.name    = name;
    l.identity.type    = layer_type_from_string(type);
    l.identity.enabled = true;
    l.identity.visible = true;
    
    l.playback.timing.start     = p.in_point;
    l.playback.timing.source_in = p.in_point;
    l.playback.timing.duration  = std::max(0.01, p.out_point - p.in_point);
    
    l.transform.width            = static_cast<int>(p.w);
    l.transform.height           = static_cast<int>(p.h);
    l.transform.opacity          = static_cast<double>(p.opacity);
    
    l.transform.transform.position_x = static_cast<double>(p.x);
    l.transform.transform.position_y = static_cast<double>(p.y);
    l.transform.transform.scale_x    = 1.0;
    l.transform.transform.scale_y    = 1.0;
    l.transform.transform.rotation   = 0.0;

    return l;
}

void apply_layer_transitions(
    LayerSpec& layer,
    const std::string& enter_id,
    double enter_duration,
    const std::string& exit_id,
    double exit_duration,
    const TransitionPresetRegistry& registry) {
    
    if (!enter_id.empty()) {
        registry::ParameterBag bag;
        bag.set("id", enter_id);
        bag.set("duration", enter_duration);
        layer.transition_in = registry.create(enter_id, bag);
    }
    
    if (!exit_id.empty()) {
        registry::ParameterBag bag;
        bag.set("id", exit_id);
        bag.set("duration", exit_duration);
        layer.transition_out = registry.create(exit_id, bag);
    }
}

} // namespace tachyon::presets
