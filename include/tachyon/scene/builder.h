#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/schema/objects/composition_spec.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/properties/property_spec.h"
#include "tachyon/core/types/colors.h"
#include "tachyon/core/animation/easing.h"
#include <initializer_list>
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <nlohmann/json.hpp>

namespace tachyon::scene {

class LayerBuilder;
class CompositionBuilder;
class SceneBuilder;

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
class TransitionBuilder {
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
class MaterialBuilder {
    LayerBuilder& parent_;
public:
    explicit MaterialBuilder(LayerBuilder& parent) : parent_(parent) {}

    MaterialBuilder& base_color(const ColorSpec& c);
    MaterialBuilder& metallic(double v);
    MaterialBuilder& roughness(double v);
    
    LayerBuilder& done();
};

/**
 * @brief Ergonomic builder for LayerSpec.
 */
class LayerBuilder {
    friend class MaterialBuilder;
    LayerSpec spec_;
public:
    explicit LayerBuilder(std::string id);

    LayerBuilder& type(std::string t);
    LayerBuilder& text(std::string t);
    LayerBuilder& font(std::string f);
    LayerBuilder& font_size(double sz);
    LayerBuilder& in(double t);
    LayerBuilder& out(double t);
    LayerBuilder& opacity(double v);
    LayerBuilder& opacity(AnimatedScalarSpec anim_spec);
    LayerBuilder& position(double x, double y);
    LayerBuilder& color(const ColorSpec& c);
    LayerBuilder& mesh(std::string path);
    LayerBuilder& prop(std::string key, nlohmann::json val);

    TransitionBuilder enter();
    TransitionBuilder exit();
    MaterialBuilder material();

    [[nodiscard]] LayerSpec build() &&;
    [[nodiscard]] LayerSpec build() const &;
};

/**
 * @brief Ergonomic builder for CompositionSpec.
 */
class CompositionBuilder {
    CompositionSpec spec_;
public:
    explicit CompositionBuilder(std::string id);

    CompositionBuilder& size(int w, int h);
    CompositionBuilder& fps(int f);
    CompositionBuilder& duration(double d);
    CompositionBuilder& background(std::string color_or_preset);

    /**
     * @brief Adds a layer using a configuration lambda.
     * The LayerBuilder is passed to the lambda and its spec is collected after execution.
     */
    CompositionBuilder& layer(std::string id, std::function<void(LayerBuilder&)> fn);

    /**
     * @brief Sets an input property (Remotion style).
     */
    template<typename T>
    CompositionBuilder& prop(std::string key, T value) {
        spec_.input_props[key] = value;
        return *this;
    }

    CompositionBuilder& audio(std::string path, double volume = 1.0);

    [[nodiscard]] CompositionSpec build() &&;
    [[nodiscard]] CompositionSpec build() const &;

    /**
     * @brief Convenience method to wrap the composition in a SceneSpec.
     */
    [[nodiscard]] SceneSpec build_scene();
};

/**
 * @brief Ergonomic builder for multi-composition SceneSpec.
 */
class SceneBuilder {
    SceneSpec spec_;
public:
    SceneBuilder& project(std::string id, std::string name);
    SceneBuilder& composition(std::string id, std::function<void(CompositionBuilder&)> fn);
    
    [[nodiscard]] SceneSpec build();
};

/**
 * @brief Main entry point for creating a composition.
 */
CompositionBuilder Composition(std::string id);

} // namespace tachyon::scene
