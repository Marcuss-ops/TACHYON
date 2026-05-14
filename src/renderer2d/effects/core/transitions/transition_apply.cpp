#include "tachyon/renderer2d/effects/core/transitions/transition_apply.h"
#include "tachyon/core/transition/transition_effect_resolver.h"
#include "tachyon/transition_registry.h"
#include <memory>

namespace tachyon {

TransitionApplyResult apply_transition(
    const TransitionApplyRequest& request,
    const TransitionRegistry& registry) {
    
    TransitionApplyResult result;
    
    // 1. Resolve the transition
    TransitionEffectResolver resolver{registry};
    TransitionEffectRequest effect_request;
    effect_request.transition_id = request.transition_id;
    effect_request.preferred_backend = TransitionRuntimeKind::CpuPixel;
    
    auto resolved = resolver.resolve(effect_request);
    
    if (!resolved.valid) {
        result.ok = false;
        result.error_message = resolved.diagnostics.error_message;
        return result;
    }
    
    if (!request.from && !request.to) {
        result.ok = false;
        result.error_message = "Both 'from' and 'to' surfaces are null.";
        return result;
    }
    
    using namespace renderer2d;
    const std::uint32_t width = request.from ? request.from->width() : request.to->width();
    const std::uint32_t height = request.from ? request.from->height() : request.to->height();
    const renderer2d::ColorProfile profile = request.from ? request.from->profile() : request.to->profile();

    // 2. Prepare output surface
    result.output.reset(width, height);
    result.output.set_profile(profile);
    
    // 3. Apply the kernel
    if (resolved.kernel.valid && resolved.kernel.apply) {
        const renderer2d::SurfaceRGBA* src_ptr = request.from;
        std::unique_ptr<renderer2d::SurfaceRGBA> dummy_from;
        if (!src_ptr) {
            dummy_from = std::make_unique<renderer2d::SurfaceRGBA>(width, height);
            dummy_from->clear(renderer2d::Color::transparent());
            dummy_from->set_profile(profile);
            src_ptr = dummy_from.get();
        }

        const renderer2d::SurfaceRGBA* tgt_ptr = request.to;
        std::unique_ptr<renderer2d::SurfaceRGBA> dummy_to;
        if (!tgt_ptr) {
            dummy_to = std::make_unique<renderer2d::SurfaceRGBA>(width, height);
            dummy_to->clear(renderer2d::Color::transparent());
            dummy_to->set_profile(profile);
            tgt_ptr = dummy_to.get();
        }
        
        resolved.kernel.apply(result.output, *src_ptr, tgt_ptr, request.progress, request.thread_count);
        result.ok = true;
    } else {
        result.ok = false;
        result.error_message = "Resolved kernel is invalid or has no apply function.";
    }
    
    return result;
}

} // namespace tachyon
