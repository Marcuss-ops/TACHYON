#pragma once

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

/// Applies a 3D LUT loaded from a .cube file.
/// Reads params.strings["lut_path"] for the file path.
/// Reads params.scalars["lut_amount"] (0-1, default 1) for blend amount.
/// Caches parsed LUTs by path to avoid re-parsing every frame.
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

class Lut3DCubeEffect : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;

private:
    mutable std::mutex m_cache_mutex;
    mutable std::unordered_map<std::string, Lut3D> m_lut_cache;
};

}  // namespace tachyon::renderer2d
