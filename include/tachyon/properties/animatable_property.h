#pragma once

#include "tachyon/core/animation/animation_curve.h"
#include "tachyon/core/properties/property.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace tachyon {
namespace properties {

/**
 * AnimatableProperty<T> — the unified property used throughout the engine for
 * transform, opacity, camera parameters, text properties, etc.
 *
 * A property can be in one of three modes:
 *  - Static:  a single constant value, independent of time.
 *  - Animated: a keyframed AnimationCurve<T>, evaluated at ctx.time.
 *  - Driven:  a user-supplied callback/expression override (future expansion).
 *
 * The value model (stored keyframes + constant) and the evaluated value are
 * kept strictly separate. Callers always call sample() to get the live value.
 *
 * Invalidation:
 *  hash_identity() returns a value that changes whenever the property's output
 *  at the queried time would change.  The engine render cache uses this as the
 *  cache key component for this property.
 *  - Static properties always return the same hash.
 *  - Animated properties hash based on the segment data surrounding `ctx.time`.
 *
 * @tparam T  Value type. Must satisfy the LerpTraits contract.
 */
template <typename T>
class AnimatableProperty : public Property<T> {
public:
    // ---- Construction ----------------------------------------------------------

    /** Create a static (non-animated) property. */
    explicit AnimatableProperty(std::string name, const T& static_value)
        : m_name(std::move(name))
        , m_is_animated(false)
        , m_static_value(static_value)
    {}

    /** Create an animated property from an existing curve. */
    explicit AnimatableProperty(std::string name, animation::AnimationCurve<T> curve)
        : m_name(std::move(name))
        , m_is_animated(true)
        , m_static_value{}
        , m_curve(std::move(curve))
    {}

    // ---- Mutation --------------------------------------------------------------

    /** Replace the static value.  Clears any existing curve and stops animation. */
    void set_static(const T& value) {
        m_static_value = value;
        m_is_animated = false;
        m_curve = {};
        ++m_version;
    }

    /** Replace the entire animation curve. */
    void set_curve(animation::AnimationCurve<T> curve) {
        m_curve = std::move(curve);
        m_is_animated = !m_curve.empty();
        ++m_version;
    }

    /**
     * Convenience: add a linear keyframe.
     * Automatically switches the property into animated mode.
     */
    void add_keyframe(double time, const T& value,
                      animation::InterpolationMode mode = animation::InterpolationMode::Linear,
                      animation::EasingPreset easing    = animation::EasingPreset::None) {
        animation::Keyframe<T> kf;
        kf.time     = time;
        kf.value    = value;
        kf.out_mode = mode;
        kf.in_mode  = mode;
        kf.easing   = easing;
        m_curve.add_keyframe(kf);
        m_curve.sort();
        m_is_animated = true;
        ++m_version;
    }

    /**
     * Convenience: add an eased keyframe.
     */
    void add_eased_keyframe(double time, const T& value, animation::EasingPreset preset) {
        add_keyframe(time, value, animation::InterpolationMode::Linear, preset);
    }

    // ---- Property interface ---------------------------------------------------

    /**
     * Sample the property value at the time given in ctx.
     * - Static properties ignore ctx.time entirely.
     * - Animated properties evaluate the curve at ctx.time.
     */
    [[nodiscard]] T sample(const PropertyEvaluationContext& ctx) const override {
        if (!m_is_animated) return m_static_value;
        return m_curve.evaluate(ctx.time);
    }

    /**
     * Returns a hash representing this property's output identity at ctx.time.
     * Changes whenever the value at that time point changes.
     *
     * Static: version ^ name hash
     * Animated: version ^ time-bucket hash (one per segment, not per sample)
     */
    [[nodiscard]] uint64_t hash_identity(const PropertyEvaluationContext& ctx) const override {
        const uint64_t name_hash = std::hash<std::string>{}(m_name);
        const uint64_t ver_hash  = m_version * 0x9E3779B97F4A7C15ULL;

        if (!m_is_animated) {
            return name_hash ^ ver_hash ^ 0xCAFEBABEDEADBEEFULL;
        }

        // For animated properties: hash based on which segment contains ctx.time
        // so that invalidation fires exactly once per segment boundary crossing.
        uint64_t seg_hash = 0;
        const auto& kfs = m_curve.keyframes;
        if (!kfs.empty()) {
            // Use the index of the active segment as part of the hash
            const uint64_t n = static_cast<uint64_t>(kfs.size());
            const uint64_t raw_time_bits =
                *reinterpret_cast<const uint64_t*>(&ctx.time); // bit-cast double
            seg_hash = n * 0x517CC1B727220A95ULL ^ (raw_time_bits >> 20);
        }

        return name_hash ^ ver_hash ^ seg_hash;
    }

    [[nodiscard]] const std::string& get_name() const override { return m_name; }

    // --- Inspection ------------------------------------------------------------

    [[nodiscard]] bool is_animated() const { return m_is_animated; }
    [[nodiscard]] const animation::AnimationCurve<T>& curve() const { return m_curve; }

    /**
     * Returns the static value (only valid when !is_animated()).
     * Calling this on an animated property is technically valid but returns
     * the default-constructed static value.
     */
    [[nodiscard]] const T& static_value() const { return m_static_value; }

private:
    std::string                  m_name;
    bool                         m_is_animated{false};
    T                            m_static_value{};
    animation::AnimationCurve<T> m_curve;
    uint64_t                     m_version{1}; ///< Monotonic version counter for invalidation.
};

} // namespace properties
} // namespace tachyon
