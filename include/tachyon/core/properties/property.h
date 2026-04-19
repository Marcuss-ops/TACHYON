#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace tachyon {
namespace properties {

/**
 * Context for property evaluation.
 */
struct PropertyEvaluationContext {
    double time{0.0};
    uint64_t seed{0};
};

/**
 * Base class for all animatable properties.
 * T is the resolved type (e.g. float, Vector3).
 */
template <typename T>
class Property {
public:
    virtual ~Property() = default;

    /**
     * Samples the value of the property at the given time.
     */
    [[nodiscard]] virtual T sample(const PropertyEvaluationContext& ctx) const = 0;

    /**
     * Returns a deterministic hash representing the identity of this value 
     * at the given time (used for caching and invalidation).
     */
    [[nodiscard]] virtual uint64_t hash_identity(const PropertyEvaluationContext& ctx) const = 0;

    /**
     * Metadata/Diagnostics info.
     */
    [[nodiscard]] virtual const std::string& get_name() const = 0;
};

/**
 * Implementation for a property that never changes (static value).
 */
template <typename T>
class ConstantProperty : public Property<T> {
public:
    ConstantProperty(const std::string& name, const T& value) 
        : name_(name), value_(value) {}

    [[nodiscard]] T sample(const PropertyEvaluationContext&) const override {
        return value_;
    }

    [[nodiscard]] uint64_t hash_identity(const PropertyEvaluationContext&) const override {
        // A constant property always has the same identity hash
        // In a real engine, this would be a precomputed hash of the value
        return 0xCAFEBABEDEADBEEL; 
    }

    [[nodiscard]] const std::string& get_name() const override {
        return name_;
    }

private:
    std::string name_;
    T value_;
};

} // namespace properties
} // namespace tachyon
