#pragma once

#include "tachyon/properties/animatable_property.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/animation/animation_curve.h"

#include <string>
#include <unordered_map>
#include <any>
#include <stdexcept>
#include <utility>

namespace tachyon {
namespace properties {

/**
 * PropertyGroup — a named, typed container of AnimatableProperty<T> instances.
 *
 * Used as the single canonical way to attach animatable state to scene nodes
 * (transforms, camera rigs, compositing layers, text animators, etc.).
 *
 * Design decisions:
 *  - Properties are type-erased via std::any in the storage map so that
 *    heterogeneous types can coexist in a single group without shared ownership.
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
        m_props[prop_name] = AnimatableProperty<T>(prop_name, default_value);
    }

    /**
     * Declare an animated property from an existing curve.
     */
    template <typename T>
    void declare_animated(const std::string& prop_name,
                          animation::AnimationCurve<T> curve) {
        m_props[prop_name] = AnimatableProperty<T>(prop_name, std::move(curve));
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
        auto* typed = std::any_cast<AnimatableProperty<T>>(&it->second);
        if (!typed) {
            throw std::runtime_error("PropertyGroup: wrong property type for '" + prop_name + "'");
        }
        return *typed;
    }

    template <typename T>
    const AnimatableProperty<T>& get(const std::string& prop_name) const {
        auto it = m_props.find(prop_name);
        if (it == m_props.end())
            throw std::runtime_error("PropertyGroup: unknown property '" + prop_name + "'");
        const auto* typed = std::any_cast<AnimatableProperty<T>>(&it->second);
        if (!typed) {
            throw std::runtime_error("PropertyGroup: wrong property type for '" + prop_name + "'");
        }
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

    std::unordered_map<std::string, std::any> m_props;
};

// ---------------------------------------------------------------------------
// Convenience: a standard SpatialTransform property group
// ---------------------------------------------------------------------------
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


} // namespace properties
} // namespace tachyon
