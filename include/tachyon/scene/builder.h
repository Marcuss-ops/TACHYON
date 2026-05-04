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
#include <memory>
#include <utility>
#include <optional>

#include "tachyon/renderer2d/effects/generators/light_leak_types.h"

namespace tachyon::scene {
using renderer2d::LightLeakPreset;

class LayerBuilder;
class CompositionBuilder;
class SceneBuilder;

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
    LayerSpec spec_;
public:
    explicit LayerBuilder(std::string id);
    explicit LayerBuilder(LayerSpec spec);

    LayerBuilder& type(std::string t);
    LayerBuilder& kind(LayerType t);
    LayerBuilder& solid(std::string name);
    LayerBuilder& image(std::string path);
    LayerBuilder& mesh(std::string path);
    LayerBuilder& preset(std::string name);
    LayerBuilder& text(std::string t);
    LayerBuilder& font(std::string f);
    LayerBuilder& font_size(double sz);
    LayerBuilder& in(double t);
    LayerBuilder& out(double t);
    LayerBuilder& opacity(double v);
    LayerBuilder& opacity(const AnimatedScalarSpec& anim_spec);
    LayerBuilder& position(double x, double y);
    LayerBuilder& size(double w, double h);
    LayerBuilder& color(const ColorSpec& c);
    LayerBuilder& fill_color(const ColorSpec& c);
    LayerBuilder& stroke_color(const ColorSpec& c);
    LayerBuilder& stroke_width(double w);
    LayerBuilder& subtitle_path(std::string path);
    LayerBuilder& text_animation_preset(const std::string& id);
    LayerBuilder& transition_in_preset(const std::string& id, double duration = 0.4);
    LayerBuilder& transition_out_preset(const std::string& id, double duration = 0.4);

    // Cinematic & Effects
    LayerBuilder& light_leak(LightLeakPreset preset, float progress, float speed = 1.0f, int seed = 3);

    // AE-like Ergonomics
    LayerBuilder& null_layer();
    LayerBuilder& precomp(std::string composition_id);
    LayerBuilder& adjustment(bool enabled = true);
    LayerBuilder& track_matte(std::string layer_id, TrackMatteType type);
    LayerBuilder& parent(std::string parent_id);
    LayerBuilder& motion_blur(bool enabled = true);

    // Text animators and highlights
    LayerBuilder& text_animator(const TextAnimatorSpec& anim);
    LayerBuilder& text_animators(std::vector<TextAnimatorSpec> anims);
    LayerBuilder& text_highlight(const TextHighlightSpec& hl);
    LayerBuilder& text_highlights(std::vector<TextHighlightSpec> hls);

    // 3D Transform
    LayerBuilder& position3d(double x, double y, double z);
    LayerBuilder& rotation3d(double x, double y, double z);
    LayerBuilder& scale3d(double x, double y, double z);
    LayerBuilder& is_3d(bool v);

    // Camera specific
    LayerBuilder& camera_type(std::string t);
    LayerBuilder& camera_poi(double x, double y, double z);
    LayerBuilder& camera_zoom(double z);

    // Light specific
    LayerBuilder& light_type(std::string t);
    LayerBuilder& light_color(const ColorSpec& c);
    LayerBuilder& light_intensity(double i);
    LayerBuilder& casts_shadows(bool v);
    LayerBuilder& shadow_radius(double r);

    // 3D Material extensions
    LayerBuilder& emission_strength(double v);

    // Transitions
    TransitionBuilder enter();
    TransitionBuilder exit();

    // Material (for 3D)
    MaterialBuilder material();

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
