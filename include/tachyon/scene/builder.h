#pragma once

#include "tachyon/core/api.h"
#include "tachyon/core/spec/schema/objects/background_spec.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/schema/objects/composition_spec.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/audio/audio_spec.h"
#include "tachyon/core/spec/schema/properties/property_spec.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/core/types/colors.h"
#include "tachyon/core/animation/easing.h"
#include "tachyon/core/math/algebra/vector3.h"
#include "tachyon/core/math/algebra/matrix4x4.h"

#include <initializer_list>
#include <functional>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <utility>
#include <optional>

#include "tachyon/renderer2d/effects/generators/light_leak_types.h"

#include "tachyon/scene/camera_builder.h"
#include "tachyon/scene/light_builder.h"
#include "tachyon/scene/text_builder.h"
#include "tachyon/scene/effect_builder.h"
#include "tachyon/scene/transform3d_builder.h"

namespace tachyon::scene {
using renderer2d::LightLeakPreset;

class LayerBuilder;
class CompositionBuilder;
class SceneBuilder;
class CameraBuilder;
class LightBuilder;
class TextBuilder;
class EffectBuilder;
class Transform3DBuilder;

namespace expr {

/**
 * @brief Simple wiggle expression.
 * @param frequency Occurrences per second.
 * @param amplitude Amount of variation.
 * @param seed Random seed for deterministic output.
 */
AnimatedScalarSpec wiggle(double frequency, double amplitude, int seed = 0);

/**
 * @brief Sine wave expression.
 * @param frequency Cycles per second.
 * @param amplitude Peak value.
 * @param offset Phase offset.
 */
AnimatedScalarSpec sin_wave(double frequency, double amplitude, double offset = 0.0);

/**
 * @brief Pulse expression (periodic on/off or scaling).
 */
AnimatedScalarSpec pulse(double frequency, double amplitude);

} // namespace expr

namespace anim {
    /**
     * @brief Creates a static scalar value.
     */
    AnimatedScalarSpec scalar(double v);

    /**
     * @brief Creates a linear interpolation (lerp) from 'from' to 'to' over 'duration'.
     */
    AnimatedScalarSpec lerp(double from, double to, double duration,
                            animation::EasingPreset ease = animation::EasingPreset::EaseInOut);

    /**
     * @brief Creates an animated scalar from explicit time-value pairs.
     */
    AnimatedScalarSpec keyframes(std::initializer_list<std::pair<double, double>> time_value_pairs);

    /**
     * @brief Samples a C++ function N times to create a keyframed animation.
     */
    AnimatedScalarSpec from_fn(double duration, std::function<double(double t)> fn, int samples = 30);
}

/**
 * @brief Builder for layer transitions (enter/exit).
 */
class TACHYON_API TransitionBuilder {
    LayerTransitionSpec& spec_;
    LayerBuilder& parent_;
public:
    explicit TransitionBuilder(LayerTransitionSpec& spec, LayerBuilder& parent) 
        : spec_(spec), parent_(parent) {}

    TransitionBuilder& id(std::string transition_id);
    TransitionBuilder& duration(double d);
    TransitionBuilder& ease(animation::EasingPreset e);
    TransitionBuilder& delay(double d);
    LayerBuilder& done();
};

/**
 * @brief Helper for the 3D material-related fields exposed on LayerSpec.
 */
class TACHYON_API MaterialBuilder {
    LayerBuilder& parent_;
public:
    explicit MaterialBuilder(LayerBuilder& parent) : parent_(parent) {}

    MaterialBuilder& base_color(const ColorSpec& c);
    MaterialBuilder& metallic(double v);
    MaterialBuilder& roughness(double v);
    MaterialBuilder& transmission(double v);
    MaterialBuilder& ior(double v);
    
    LayerBuilder& done();
};

/**
 * @brief Ergonomic builder for LayerSpec.
 */
class TACHYON_API LayerBuilder {
    friend class MaterialBuilder;
    friend class CameraBuilder;
    friend class LightBuilder;
    friend class TextBuilder;
    friend class EffectBuilder;
    friend class Transform3DBuilder;
    LayerSpec spec_;
public:
    explicit LayerBuilder(std::string id);
    explicit LayerBuilder(LayerSpec spec);

    // Basic properties
    LayerBuilder& type(LayerType t);
    LayerBuilder& solid(std::string name);
    LayerBuilder& image(std::string path);
    LayerBuilder& mesh(std::string path);
    LayerBuilder& preset(std::string name);
    LayerBuilder& asset_id(std::string id);
    LayerBuilder& mesh_deform_id(std::optional<std::string> id);
    LayerBuilder& clear_mesh_deform_id();
    LayerBuilder& in(double t);
    LayerBuilder& out(double t);

    // 2D Transform
    LayerBuilder& opacity(double v);
    LayerBuilder& opacity(const AnimatedScalarSpec& anim_spec);
    LayerBuilder& position(double x, double y);
    LayerBuilder& size(double w, double h);
    LayerBuilder& color(const ColorSpec& c);
    LayerBuilder& fill_color(const ColorSpec& c);
    LayerBuilder& stroke_color(const ColorSpec& c);
    LayerBuilder& stroke_width(double w);

    // AE-like Ergonomics
    LayerBuilder& null_layer();
    LayerBuilder& precomp(std::string composition_id);
    LayerBuilder& adjustment(bool enabled = true);
    LayerBuilder& track_matte(std::string layer_id, TrackMatteType type);
    LayerBuilder& parent(std::string parent_id);
    LayerBuilder& motion_blur(bool enabled = true);

    // Transitions
    TransitionBuilder enter();
    TransitionBuilder exit();

    // Material (for 3D)
    MaterialBuilder material();

    // Domain-specific builders
    CameraBuilder camera();
    LightBuilder light();
    TextBuilder text();
    EffectBuilder effects();
    Transform3DBuilder transform3d();

    // 2D Animation preset
    LayerBuilder& animation2d_preset(const std::string& id, double duration = 1.0, double delay = 0.0);

    [[nodiscard]] LayerSpec build() &&;
    [[nodiscard]] LayerSpec build() const &;
};

/**
 * @brief Ergonomic builder for CompositionSpec.
 */
class TACHYON_API CompositionBuilder {
    CompositionSpec spec_;

    // Private helper for typed layer methods
    CompositionBuilder& add_typed_layer(
        std::string id,
        std::function<void(LayerBuilder&)> defaults,
        std::function<void(LayerBuilder&)> fn);
public:
    explicit CompositionBuilder(std::string id);

    CompositionBuilder& size(int w, int h);
    CompositionBuilder& fps(int f);
    CompositionBuilder& duration(double d);
    CompositionBuilder& background(BackgroundSpec spec);
    CompositionBuilder& background_preset(const std::string& id, double duration = 8.0);
    CompositionBuilder& clear(const ColorSpec& color);

    CompositionBuilder& layer(std::string id, std::function<void(LayerBuilder&)> fn);
    CompositionBuilder& layer(const LayerSpec& layer);  // Accept LayerSpec directly
    
    // AE-like ergonomic layers
    CompositionBuilder& null_layer(std::string id, std::function<void(LayerBuilder&)> fn);
    CompositionBuilder& precomp_layer(std::string id, std::string composition_id, std::function<void(LayerBuilder&)> fn);

    CompositionBuilder& camera3d_layer(std::string id, std::function<void(LayerBuilder&)> fn);
    CompositionBuilder& light_layer(std::string id, std::function<void(LayerBuilder&)> fn);
    CompositionBuilder& mesh_layer(std::string id, std::function<void(LayerBuilder&)> fn);
    CompositionBuilder& audio(std::string path, double volume = 1.0);
    CompositionBuilder& audio(const AudioTrackSpec& track);  // Accept AudioTrackSpec directly

    [[nodiscard]] CompositionSpec build() &&;
    [[nodiscard]] CompositionSpec build() const &;

    [[nodiscard]] SceneSpec build_scene();
};

/**
 * @brief Ergonomic builder for multi-composition SceneSpec.
 */
class TACHYON_API SceneBuilder {
    SceneSpec spec_;
public:
    SceneBuilder& project(std::string id, std::string name);
    SceneBuilder& composition(std::string id, std::function<void(CompositionBuilder&)> fn);
    
    [[nodiscard]] SceneSpec build();
};

/**
 * @brief Main entry point for creating a composition.
 */
TACHYON_API CompositionBuilder Composition(std::string id);

/**
 * @brief Main entry point for creating a multi-composition scene.
 */
TACHYON_API SceneBuilder Scene();

} // namespace tachyon::scene
