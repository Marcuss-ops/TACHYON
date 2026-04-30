#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <optional>

namespace tachyon {
namespace properties {

/**
 * Context for property evaluation.
 */
struct PropertyEvaluationContext {
    double time{0.0};
    uint64_t seed{0};
};

enum class PropertyType : uint8_t {
    Constant,
    Animatable,
    Expression
};

// Forward declarations of implementation details
template <typename T> class AnimatableProperty;
template <typename T> class ExpressionProperty;

/**
 * @brief Unified, non-virtual animatable property.
 * 
 * Replaces the old virtual Property<T> base class to eliminate virtual 
 * dispatch overhead in the hot render loop. Uses a internal variant 
 * to handle different animation modes.
 */
template <typename T>
class Property {
public:
    Property() : m_type(PropertyType::Constant), m_constant(), m_name("unnamed") {}
    
    /** Create a constant property. */
    Property(std::string name, T value) 
        : m_type(PropertyType::Constant), m_constant(std::move(value)), m_name(std::move(name)) {}

    /** Create from an implementation (Animatable or Expression). */
    Property(std::string name, std::shared_ptr<AnimatableProperty<T>> anim)
        : m_type(PropertyType::Animatable), m_animatable(std::move(anim)), m_name(std::move(name)) {}
    
    Property(std::string name, std::shared_ptr<ExpressionProperty<T>> expr)
        : m_type(PropertyType::Expression), m_expression(std::move(expr)), m_name(std::move(name)) {}

    /**
     * Samples the value of the property at the given time.
     * Non-virtual switch/dispatch is significantly faster than virtual calls.
     */
    [[nodiscard]] T sample(const PropertyEvaluationContext& ctx) const;

    /**
     * Returns a deterministic hash representing the identity of this value 
     * at the given time (used for caching and invalidation).
     */
    [[nodiscard]] uint64_t hash_identity(const PropertyEvaluationContext& ctx) const;

    [[nodiscard]] const std::string& get_name() const { return m_name; }
    [[nodiscard]] PropertyType type() const { return m_type; }

    // Helpers to access internals if needed
    [[nodiscard]] const T& constant_value() const { return m_constant; }
    [[nodiscard]] std::shared_ptr<AnimatableProperty<T>> animatable() const { return m_animatable; }
    [[nodiscard]] std::shared_ptr<ExpressionProperty<T>> expression() const { return m_expression; }

private:
    PropertyType m_type;
    T m_constant;
    std::shared_ptr<AnimatableProperty<T>> m_animatable;
    std::shared_ptr<ExpressionProperty<T>> m_expression;
    std::string m_name;
};

} // namespace properties
} // namespace tachyon

// Include implementations to avoid circular dependencies where possible
#include "tachyon/properties/animatable_property.h"
#include "tachyon/core/properties/expression_property.h"

namespace tachyon::properties {

template <typename T>
T Property<T>::sample(const PropertyEvaluationContext& ctx) const {
    switch (m_type) {
        case PropertyType::Animatable:
            if (m_animatable) return m_animatable->sample(ctx);
            break;
        case PropertyType::Expression:
            if (m_expression) return m_expression->sample(ctx);
            break;
        case PropertyType::Constant:
        default:
            break;
    }
    return m_constant;
}

template <typename T>
uint64_t Property<T>::hash_identity(const PropertyEvaluationContext& ctx) const {
    switch (m_type) {
        case PropertyType::Animatable:
            if (m_animatable) return m_animatable->hash_identity(ctx);
            break;
        case PropertyType::Expression:
            if (m_expression) return m_expression->hash_identity(ctx);
            break;
        case PropertyType::Constant:
        default:
            break;
    }
    return 0xCAFEBABEDEADBEEFULL ^ std::hash<std::string>{}(m_name);
}

} // namespace tachyon::properties
