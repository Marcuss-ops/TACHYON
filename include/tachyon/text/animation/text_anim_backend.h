#pragma once

#include "tachyon/text/animation/text_anim_property_registry.h"
#include "tachyon/text/animation/text_animator_utils.h"
#include <vector>
#include <memory>
#include <span>

namespace tachyon::text {

struct PrecompGlyphCtx {
    std::size_t cluster_index;
    std::size_t word_index;
    std::size_t line_index;
    bool is_space;
    bool is_rtl;
    std::size_t cluster_codepoint_start;
    std::size_t cluster_codepoint_count;
    float non_space_index;
};

struct ResolvedTextAnimPlan {
    std::vector<ResolvedTextAnimProperty> resolved_properties;
    
    static ResolvedTextAnimPlan resolve(const TextAnimatorSpec& animator) {
        ResolvedTextAnimPlan plan;
        const auto& registry = TextAnimPropertyRegistry::instance();
        plan.resolved_properties = registry.resolve_properties(animator.properties);
        return plan;
    }
};

class TextAnimExecutionBackend {
public:
    virtual ~TextAnimExecutionBackend() = default;
    
    virtual void apply(
        const ResolvedTextAnimPlan& plan,
        std::span<PositionedGlyph> glyphs,
        const TextAnimatorSpec& animator,
        const TextAnimatorContext& base_ctx,
        std::span<const PrecompGlyphCtx> precomp,
        float t,
        float stagger_delay,
        const std::string& stagger_mode) = 0;
    
    virtual std::string name() const = 0;
    
    static std::unique_ptr<TextAnimExecutionBackend> create_scalar();
    static std::unique_ptr<TextAnimExecutionBackend> create_avx2();
    static std::unique_ptr<TextAnimExecutionBackend> create_openmp();
    static std::unique_ptr<TextAnimExecutionBackend> create_best();
};

class ScalarTextAnimBackend : public TextAnimExecutionBackend {
public:
    void apply(
        const ResolvedTextAnimPlan& plan,
        std::span<PositionedGlyph> glyphs,
        const TextAnimatorSpec& animator,
        const TextAnimatorContext& base_ctx,
        std::span<const PrecompGlyphCtx> precomp,
        float t,
        float stagger_delay,
        const std::string& stagger_mode) override;
    
    std::string name() const override { return "scalar"; }
};

class OpenMpTextAnimBackend : public TextAnimExecutionBackend {
public:
    void apply(
        const ResolvedTextAnimPlan& plan,
        std::span<PositionedGlyph> glyphs,
        const TextAnimatorSpec& animator,
        const TextAnimatorContext& base_ctx,
        std::span<const PrecompGlyphCtx> precomp,
        float t,
        float stagger_delay,
        const std::string& stagger_mode) override;
    
    std::string name() const override { return "openmp"; }
};

#ifdef TACHYON_AVX2
class Avx2TextAnimBackend : public TextAnimExecutionBackend {
public:
    void apply(
        const ResolvedTextAnimPlan& plan,
        std::span<PositionedGlyph> glyphs,
        const TextAnimatorSpec& animator,
        const TextAnimatorContext& base_ctx,
        std::span<const PrecompGlyphCtx> precomp,
        float t,
        float stagger_delay,
        const std::string& stagger_mode) override;
    
    std::string name() const override { return "avx2"; }
};
#endif

} // namespace tachyon::text
