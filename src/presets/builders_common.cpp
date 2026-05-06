#include "tachyon/presets/builders_common.h"
#include "tachyon/presets/transition/transition_preset_registry.h"

namespace tachyon::presets {

LayerSpec make_base_layer(
    const std::string& id,
    const std::string& name,
    const std::string& type,
    const LayerParams& p) {
    
    LayerSpec l;
    l.id          = id;
    l.name        = name;
    l.type        = layer_type_from_string(type);
    l.type_string.clear();
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
    
    auto& registry = TransitionPresetRegistry::instance();

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
