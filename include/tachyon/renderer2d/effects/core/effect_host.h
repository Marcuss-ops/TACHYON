#pragma once

#include "tachyon/renderer2d/raster/draw_command.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include "tachyon/renderer2d/color/lut3d.h"
#include "tachyon/renderer2d/raster/rasterizer.h"
#include "tachyon/renderer2d/core/framebuffer.h"

#include <array>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace tachyon::renderer2d {

class Effect {
public:
    virtual ~Effect() = default;
    virtual SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const = 0;
};

class EffectHost {
 public:
  EffectHost() = default;
  virtual ~EffectHost() = default;

  virtual void register_effect(std::string name, std::unique_ptr<Effect> effect) = 0;
  virtual bool has_effect(const std::string& name) const = 0;
  virtual SurfaceRGBA apply(const std::string& name,
                            const SurfaceRGBA& input,
                            const EffectParams& params) const = 0;
  virtual SurfaceRGBA apply_pipeline(
      const SurfaceRGBA& input,
      const std::vector<std::pair<std::string, EffectParams>>& pipeline) const = 0;

  static void register_builtins(EffectHost& host);

 protected:
  std::unordered_map<std::string, std::unique_ptr<Effect>> m_effects;
};

std::unique_ptr<EffectHost> create_effect_host();

// Built-in Effect declarations
class GaussianBlurEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class DirectionalBlurEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class RadialBlurEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class DropShadowEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class GlowEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class LevelsEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class CurvesEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class FillEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class TintEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class HueSaturationEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class ColorBalanceEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class LUTEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class ChromaticAberrationEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class VignetteEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class ParticleEmitterEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class DisplacementMapEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class GlslTransitionEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class LensFlareEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class LightLeakEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class WarpStabilizerEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class Lut3DCubeEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;

private:
    mutable std::mutex m_cache_mutex;
    mutable std::unordered_map<std::string, Lut3D> m_lut_cache;
};

class ChromaKeyEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

/// Light Wrap: samples the background at the foreground edge and blends it in,
/// creating a natural-looking integration between keyed subject and plate.
/// Requires params.aux_surfaces["background"] to be set by the compositor.
class LightWrapEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

/// Matte Refinement: choke/spread, temporal denoise, and per-channel clip
/// applied to the alpha channel of an already-keyed layer.
class MatteRefinementEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;

    // Feed previous frame's alpha for temporal smoothing.
    // Key: layer_id from params.strings["layer_id"].
    static void set_previous_frame(const std::string& layer_id, std::vector<float> alpha);
    static const std::vector<float>* get_previous_frame(const std::string& layer_id);

private:
    // Per-layer alpha cache for temporal smoothing (lock-free read after write per frame)
    static std::unordered_map<std::string, std::vector<float>> s_prev_alpha;
    static std::mutex s_cache_mutex;
};

/// Vector Blur: per-pixel motion-vector driven directional blur.
/// Requires params.aux_surfaces["motion_vectors"] (RG = velocity in pixels/frame).
/// Falls back to a simple radial blur if no motion vector buffer is provided.
class VectorBlurEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

/// Motion Blur 2D: simulates motion blur by blending multiple offset samples.
/// Uses params.values["shutter_angle"] (default 180.0) and params.values["samples"] (default 8).
class MotionBlur2DEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

}  // namespace tachyon::renderer2d
