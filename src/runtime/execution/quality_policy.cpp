#include "tachyon/runtime/execution/planning/quality_policy.h"

namespace tachyon {

namespace {
    // Tile size constants
    constexpr uint32_t kTileSizeSmall  = 256;
    constexpr uint32_t kTileSizeLarge  = 512;

    // Cache budget constants (MB)
    constexpr uint64_t kMB = 1024ULL * 1024;
    constexpr uint64_t kCacheBudgetDraft      = 64 * kMB;
    constexpr uint64_t kCacheBudgetPreview    = 256 * kMB;
    constexpr uint64_t kCacheBudgetProduction = 1024 * kMB;
    constexpr uint64_t kCacheBudgetCinematic  = 2048 * kMB;

    // Motion blur constants
    constexpr float kDefaultShutterAngle = 180.0f;
}

QualityPolicy make_quality_policy(QualityTier tier) {
    QualityPolicy policy;

    switch (tier) {
        case QualityTier::Draft:
            policy.resolution_scale         = kQualityDraftResolutionScale;
            policy.motion_blur_samples      = 1;      // disabled
            policy.motion_blur_sample_cap   = 1;
            policy.max_coc_radius_px        = 8.0f;
            policy.dof_sample_count         = 4;
            policy.dof_fg_bg_separate       = false;
            policy.ray_tracer_spp           = 1;
            policy.ray_tracer_max_bounces   = 1;
            policy.denoiser_enabled         = false;
            policy.shadow_map_resolution    = 256;
            policy.soft_shadows             = false;
            policy.effects_enabled          = false;
            policy.precomp_cache_budget     = kCacheBudgetDraft;
            policy.tile_size                = kTileSizeSmall;
            policy.max_workers              = 2;
            policy.shutter_angle_deg        = kDefaultShutterAngle;
            break;

        case QualityTier::Preview:
            policy.resolution_scale         = kQualityPreviewResolutionScale;
            policy.motion_blur_samples      = 4;
            policy.motion_blur_sample_cap   = 8;
            policy.max_coc_radius_px        = 16.0f;
            policy.dof_sample_count         = 8;
            policy.dof_fg_bg_separate       = false;
            policy.ray_tracer_spp           = 1;
            policy.ray_tracer_max_bounces   = 2;
            policy.denoiser_enabled         = false;
            policy.shadow_map_resolution    = 512;
            policy.soft_shadows             = false;
            policy.effects_enabled          = true;
            policy.precomp_cache_budget     = kCacheBudgetPreview;
            policy.tile_size                = kTileSizeLarge;
            policy.max_workers              = 0; // all cores
            policy.shutter_angle_deg        = kDefaultShutterAngle;
            break;

        case QualityTier::Production:
            policy.resolution_scale         = kQualityProductionResolutionScale;
            policy.motion_blur_samples      = 8;
            policy.motion_blur_sample_cap   = 32;
            policy.max_coc_radius_px        = 24.0f;
            policy.dof_sample_count         = 16;
            policy.dof_fg_bg_separate       = true;
            policy.ray_tracer_spp           = 4;
            policy.ray_tracer_max_bounces   = 4;
            policy.denoiser_enabled         = true;
            policy.shadow_map_resolution    = 1024;
            policy.soft_shadows             = true;
            policy.effects_enabled          = true;
            policy.precomp_cache_budget     = kCacheBudgetProduction;
            policy.tile_size                = kTileSizeLarge;
            policy.max_workers              = 0;
            policy.shutter_angle_deg        = kDefaultShutterAngle;
            break;

        case QualityTier::Cinematic:
            policy.resolution_scale         = kQualityCinematicResolutionScale;
            policy.motion_blur_samples      = 16;
            policy.motion_blur_sample_cap   = 64;
            policy.max_coc_radius_px        = 32.0f;
            policy.dof_sample_count         = 32;
            policy.dof_fg_bg_separate       = true;
            policy.ray_tracer_spp           = 16;
            policy.ray_tracer_max_bounces   = 8;
            policy.denoiser_enabled         = true;
            policy.shadow_map_resolution    = 2048;
            policy.soft_shadows             = true;
            policy.effects_enabled          = true;
            policy.precomp_cache_budget     = kCacheBudgetCinematic;
            policy.tile_size                = kTileSizeSmall; // smaller tiles for better cache locality at high SPP
            policy.max_workers              = 0;
            policy.shutter_angle_deg        = kDefaultShutterAngle;
            break;
    }

    return policy;
}

QualityPolicy make_quality_policy(const std::string& tier_name) {
    return make_quality_policy(quality_tier_from_string(tier_name));
}

const char* to_string(QualityTier tier) {
    switch (tier) {
        case QualityTier::Draft:      return "draft";
        case QualityTier::Preview:    return "preview";
        case QualityTier::Production: return "production";
        case QualityTier::Cinematic:  return "cinematic";
    }
    return "unknown";
}

QualityTier quality_tier_from_string(const std::string& name) {
    if (name == "draft")                        return QualityTier::Draft;
    if (name == "preview")                      return QualityTier::Preview;
    if (name == "production" || name == "high") return QualityTier::Production;
    if (name == "cinematic")                    return QualityTier::Cinematic;

    // Manifesto Rule 6: fail-fast. Never swallow invalid input.
    throw std::invalid_argument(
        "Unknown quality tier: '" + name +
        "'. Valid tiers: draft, preview, production, cinematic.");
}

} // namespace tachyon
