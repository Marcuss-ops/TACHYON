#pragma once

#include <cstddef>
#include <string>
#include <stdexcept>

namespace tachyon {

/**
 * @brief Rendering quality tiers.
 *
 * Manifesto Rule 7: explicit names. No numeric magic tiers.
 */
enum class QualityTier {
    Draft,      ///< Fastest, lowest fidelity. For UI/scrubbing.
    Preview,    ///< Balanced quality for interactive playback.
    Production, ///< Full quality for final output.
    Cinematic   ///< Maximum quality. Slower, film-grade.
};

/* Named constants — Manifesto Rule 7: no magic numbers */
constexpr float kQualityDraftResolutionScale = 0.5f;
constexpr float kQualityPreviewResolutionScale = 0.75f;
constexpr float kQualityProductionResolutionScale = 1.0f;
constexpr float kQualityCinematicResolutionScale = 1.0f;

/**
 * @brief Deterministic, pre-computed render quality policy.
 *
 * Manifesto Rules:
 * - Rule 3: all data contiguous, zero allocations in render loop.
 * - Rule 5: bit-exact determinism — same tier always produces same policy.
 * - Rule 7: every parameter has an explicit, named field.
 */
struct QualityPolicy {
    // Resolution
    float resolution_scale{1.0f};

    // Motion Blur
    int motion_blur_samples{1};        ///< Active samples. 1 = disabled.
    int motion_blur_sample_cap{64};    ///< Hard upper bound for safety.

    // Depth of Field
    float max_coc_radius_px{20.0f};    ///< Max Circle of Confusion radius in pixels.
    int dof_sample_count{16};          ///< Bokeh blur samples.
    bool dof_fg_bg_separate{false};    ///< Separate foreground/background blur handling.

    // Ray Tracing
    int ray_tracer_spp{1};             ///< Samples per pixel.
    int ray_tracer_max_bounces{2};     ///< Max recursive bounces.
    bool denoiser_enabled{false};      ///< Post-render denoise pass.

    // Shadows
    int shadow_map_resolution{512};    ///< Shadow map texture size.
    bool soft_shadows{false};          ///< Area / PCSS shadows.

    // Caching & Execution
    bool effects_enabled{true};
    bool precomp_cache_enabled{true};
    std::size_t precomp_cache_budget{1ULL * 1024 * 1024 * 1024};
    int tile_size{0};                  ///< 0 = full frame, else tile dimension.
    std::size_t max_workers{0};        ///< 0 = use all cores.

    // Shutter
    float shutter_angle_deg{180.0f};   ///< 180° = half frame duration.
};

/* Type-safe factories — Manifesto Rule 6: fail-fast */
QualityPolicy make_quality_policy(QualityTier tier);
QualityPolicy make_quality_policy(const std::string& tier_name);

/* Conversion utilities — Manifesto Rule 7: explicit, reversible */
const char* to_string(QualityTier tier);
QualityTier quality_tier_from_string(const std::string& name);

} // namespace tachyon
