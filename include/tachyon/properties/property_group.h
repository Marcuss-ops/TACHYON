#pragma once

#include "tachyon/properties/animatable_property.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include "tachyon/core/animation/animation_curve.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <stdexcept>

namespace tachyon {
namespace properties {

/**
 * PropertyGroup — a named, typed container of AnimatableProperty<T> instances.
 *
 * Used as the single canonical way to attach animatable state to scene nodes
 * (transforms, camera rigs, compositing layers, text animators, etc.).
 *
 * Design decisions:
 *  - Properties are type-erased via shared_ptr<void> in the storage map so that
 *    heterogeneous types can coexist in a single group without virtual dispatch
 *    overhead on access (the caller knows the type at access point).
 *  - A second map stores std::string names for diagnostics/introspection.
 *  - All sample() calls are deferred to the typed AnimatableProperty<T> to
 *    maintain full determinism.
 */
class PropertyGroup {
public:
    explicit PropertyGroup(std::string name) : m_name(std::move(name)) {}

    // --- Registration ----------------------------------------------------------

    /**
     * Declare a static (constant) property in this group.
     * Replaces if a property with the same name already exists.
     */
    template <typename T>
    void declare_static(const std::string& prop_name, const T& default_value) {
        auto prop = std::make_shared<AnimatableProperty<T>>(prop_name, default_value);
        m_props[prop_name] = prop;
    }

    /**
     * Declare an animated property from an existing curve.
     */
    template <typename T>
    void declare_animated(const std::string& prop_name,
                          animation::AnimationCurve<T> curve) {
        auto prop = std::make_shared<AnimatableProperty<T>>(prop_name, std::move(curve));
        m_props[prop_name] = prop;
    }

    // --- Access ----------------------------------------------------------------

    /**
     * Access a typed property by name.
     * Throws std::runtime_error if the name is missing or has the wrong type.
     */
    template <typename T>
    AnimatableProperty<T>& get(const std::string& prop_name) {
        auto it = m_props.find(prop_name);
        if (it == m_props.end())
            throw std::runtime_error("PropertyGroup: unknown property '" + prop_name + "'");
        auto* typed = static_cast<AnimatableProperty<T>*>(it->second.get());
        return *typed;
    }

    template <typename T>
    const AnimatableProperty<T>& get(const std::string& prop_name) const {
        auto it = m_props.find(prop_name);
        if (it == m_props.end())
            throw std::runtime_error("PropertyGroup: unknown property '" + prop_name + "'");
        const auto* typed = static_cast<const AnimatableProperty<T>*>(it->second.get());
        return *typed;
    }

    /**
     * Evaluate a typed property at the given evaluation context.
     * Convenience wrapper around get<T>().sample(ctx).
     */
    template <typename T>
    [[nodiscard]] T sample(const std::string& prop_name,
                           const PropertyEvaluationContext& ctx) const {
        return get<T>(prop_name).sample(ctx);
    }

    [[nodiscard]] bool has(const std::string& prop_name) const {
        return m_props.count(prop_name) > 0;
    }

    [[nodiscard]] const std::string& name() const { return m_name; }

private:
    std::string m_name;

    // Type-erased storage: prop_name -> shared_ptr<AnimatableProperty<T>>
    // We store it as shared_ptr<void> so heterogeneous types can coexist.
    std::unordered_map<std::string, std::shared_ptr<void>> m_props;
};

// ---------------------------------------------------------------------------
// Convenience: a standard Transform3 property group
// ---------------------------------------------------------------------------

/**
 * Creates a PropertyGroup pre-populated with the standard 3D transform
 * properties: "position", "rotation_euler", "scale".
 * These names are the canonical engine-wide keys for transform state on any
 * 3D scene node.
 */
inline PropertyGroup make_transform3_group(const std::string& group_name = "transform") {
    PropertyGroup g(group_name);
    g.declare_static<math::Vector3>("position",        math::Vector3::zero());
    g.declare_static<math::Vector3>("rotation_euler",  math::Vector3::zero()); // degrees, YXZ
    g.declare_static<math::Vector3>("scale",           math::Vector3::one());
    return g;
}

/**
 * Creates a PropertyGroup pre-populated with the standard 2D compositing
 * properties: "position", "rotation", "scale", "opacity".
 */
inline PropertyGroup make_layer_group(const std::string& group_name = "layer") {
    PropertyGroup g(group_name);
    g.declare_static<math::Vector2>("position",  math::Vector2::zero());
    g.declare_static<float>("rotation",  0.0f);   // degrees
    g.declare_static<math::Vector2>("scale",     {1.0f, 1.0f});
    g.declare_static<float>("opacity",   1.0f);   // 0..1
    return g;
}

/**
 * Creates a PropertyGroup for a camera node:
 * fov_y_deg, near_clip, far_clip, focal_length, focus_distance, aperture.
 */
inline PropertyGroup make_camera_group(const std::string& group_name = "camera") {
    PropertyGroup g(group_name);
    g.declare_static<float>("fov_y_deg",       60.0f);
    g.declare_static<float>("near_clip",        0.1f);
    g.declare_static<float>("far_clip",      1000.0f);
    g.declare_static<float>("focal_length",    50.0f);  // mm
    g.declare_static<float>("focus_distance", 10.0f);   // world units
    g.declare_static<float>("aperture",        2.8f);   // f-stop
    return g;
}

} // namespace properties
} // namespace tachyon
