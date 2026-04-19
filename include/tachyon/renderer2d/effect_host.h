#pragma once

#include "tachyon/renderer2d/framebuffer.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace tachyon::renderer2d {

struct EffectParams {
    std::unordered_map<std::string, float> scalars;
    std::unordered_map<std::string, Color> colors;
};

class Effect {
public:
    virtual ~Effect() = default;
    virtual SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const = 0;
};

class GaussianBlurEffect final : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class DropShadowEffect final : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class GlowEffect final : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class FillEffect final : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class TintEffect final : public Effect {
public:
    SurfaceRGBA apply(const SurfaceRGBA& input, const EffectParams& params) const override;
};

class EffectHost {
public:
    void register_effect(std::string name, std::unique_ptr<Effect> effect);
    bool has_effect(const std::string& name) const;
    SurfaceRGBA apply(const std::string& name, const SurfaceRGBA& input, const EffectParams& params) const;

private:
    std::unordered_map<std::string, std::unique_ptr<Effect>> m_effects;
};

} // namespace tachyon::renderer2d
