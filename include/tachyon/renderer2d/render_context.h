#pragma once

#include "tachyon/renderer2d/rasterizer.h"
#include "tachyon/renderer2d/effect_host.h"
#include "tachyon/runtime/quality_policy.h"
#include "tachyon/text/font_registry.h"

#include <mutex>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace tachyon {

struct RasterizedFrame2D;

namespace renderer2d {

struct AccumulationBufferState {
    std::vector<float> r;
    std::vector<float> g;
    std::vector<float> b;
    std::vector<float> a;
    
    void ensure_capacity(std::size_t size) {
        if (r.size() < size) {
            r.resize(size);
            g.resize(size);
            b.resize(size);
            a.resize(size);
        }
    }
};

struct PrecompCache {
    std::size_t max_bytes{256ULL * 1024 * 1024};

    bool lookup(const std::string& key, RasterizedFrame2D& frame) const;
    void store(std::string key, RasterizedFrame2D frame);
    void clear();
    std::size_t entry_count() const;
    std::size_t current_bytes() const;

private:
    mutable std::mutex m_mutex;
    std::size_t m_current_bytes{0};
    std::unordered_map<std::string, RasterizedFrame2D> m_entries;
};

struct RenderContext {
    std::shared_ptr<PrecompCache> precomp_cache;
    EffectHost effects;
    QualityPolicy policy;
    text::FontRegistry fonts;
    int precomp_recursion_depth{0};
    std::shared_ptr<AccumulationBufferState> accumulation_buffer{std::make_shared<AccumulationBufferState>()};

    explicit RenderContext(std::shared_ptr<PrecompCache> cache = std::make_shared<PrecompCache>())
        : precomp_cache(std::move(cache)),
          policy{} {
        effects.register_effect("gaussian_blur", std::make_unique<GaussianBlurEffect>());
        effects.register_effect("drop_shadow", std::make_unique<DropShadowEffect>());
        effects.register_effect("glow", std::make_unique<GlowEffect>());
        effects.register_effect("levels", std::make_unique<LevelsEffect>());
        effects.register_effect("curves", std::make_unique<CurvesEffect>());
        effects.register_effect("fill", std::make_unique<FillEffect>());
        effects.register_effect("tint", std::make_unique<TintEffect>());
    }
};

} // namespace renderer2d
} // namespace tachyon
