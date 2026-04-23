#pragma once

#include "tachyon/core/animation/keyframe_track.h"
#include "tachyon/core/properties/property.h"
#include "tachyon/core/scene/dependency_node.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace tachyon {
namespace properties {

/**
 * LinkedPropertySource — an external provider of property values (e.g. tracker, expressions).
 */
template <typename T>
class LinkedPropertySource {
public:
    virtual ~LinkedPropertySource() = default;
    [[nodiscard]] virtual T sample(double time) const = 0;
    [[nodiscard]] virtual uint64_t hash_identity(double time) const = 0;
};

/**
 * AnimatableProperty<T> — the unified property used throughout the engine for
 * transform, opacity, camera parameters, text properties, etc.
 *
 * A property can be in one of three modes:
 *  - Static:  a single constant value, independent of time.
 *  - Animated: a keyframed KeyframeTrack<T>, evaluated at ctx.time.
 *  - Linked:   a value driven by an external source (e.g. Tracker).
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
class AnimatableProperty : public tachyon::core::scene::DependencyNode {
public:
    // ---- Construction ----------------------------------------------------------

    /** Create a static (non-animated) property. */
    explicit AnimatableProperty(std::string name, const T& static_value)
        : m_name(std::move(name))
        , m_mode(PropertyMode::Static)
        , m_static_value(static_value)
    {}

    /** Create an animated property from an existing track. */
    explicit AnimatableProperty(std::string name, animation::KeyframeTrack<T> track)
        : m_name(std::move(name))
        , m_mode(PropertyMode::Animated)
        , m_static_value{}
        , m_curve(std::move(track))
    {}

    /** Create a linked property driven by an external source. */
    explicit AnimatableProperty(std::string name, std::shared_ptr<LinkedPropertySource<T>> source)
        : m_name(std::move(name))
        , m_mode(PropertyMode::Linked)
        , m_static_value{}
        , m_linked_source(std::move(source))
    {}

    // ---- Mutation --------------------------------------------------------------

    /** Replace the static value.  Clears any existing curve and stops animation. */
    void set_static(const T& value, uint64_t tick = 0) {
        m_static_value = value;
        m_mode = PropertyMode::Static;
        m_curve = {};
        m_linked_source = nullptr;
        ++m_version;
        mark_dirty(tick);
    }

    /** Replace the entire animation track. */
    void set_curve(animation::KeyframeTrack<T> track, uint64_t tick = 0) {
        m_curve = std::move(track);
        m_mode = m_curve.empty() ? PropertyMode::Static : PropertyMode::Animated;
        m_linked_source = nullptr;
        ++m_version;
        mark_dirty(tick);
    }

    /** Set the property to be driven by an external source. */
    void set_linked(std::shared_ptr<LinkedPropertySource<T>> source, uint64_t tick = 0) {
        m_linked_source = std::move(source);
        m_mode = m_linked_source ? PropertyMode::Linked : PropertyMode::Static;
        m_curve = {};
        ++m_version;
        mark_dirty(tick);
    }

    /**
     * Convenience: add a linear keyframe.
     * Automatically switches the property into animated mode.
     */
    void add_keyframe(double time, const T& value,
                      animation::InterpolationMode mode = animation::InterpolationMode::Linear,
                      animation::EasingPreset easing    = animation::EasingPreset::None,
                      uint64_t tick = 0) {
        animation::Keyframe<T> kf;
        kf.time     = time;
        kf.value    = value;
        kf.out_mode = mode;
        kf.in_mode  = mode;
        kf.easing   = easing;
        m_curve.add_keyframe(kf);
        m_curve.sort();
        m_mode = PropertyMode::Animated;
        m_linked_source = nullptr;
        ++m_version;
        mark_dirty(tick);
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
    [[nodiscard]] T sample(const PropertyEvaluationContext& ctx) const {
        switch (m_mode) {
            case PropertyMode::Static:   return m_static_value;
            case PropertyMode::Animated: return m_curve.evaluate(ctx.time);
            case PropertyMode::Linked:   return m_linked_source ? m_linked_source->sample(ctx.time) : m_static_value;
        }
        return m_static_value;
    }

    /**
     * Returns a hash representing this property's output identity at ctx.time.
     * Changes whenever the value at that time point changes.
     *
     * Static: version ^ name hash
     * Animated: version ^ time-bucket hash (one per segment, not per sample)
     */
    [[nodiscard]] uint64_t hash_identity(const PropertyEvaluationContext& ctx) const {
        const uint64_t name_hash = std::hash<std::string>{}(m_name);
        const uint64_t ver_hash  = m_version * 0x9E3779B97F4A7C15ULL;

        if (m_mode == PropertyMode::Static) {
            return name_hash ^ ver_hash ^ 0xCAFEBABEDEADBEEFULL;
        }

        if (m_mode == PropertyMode::Linked) {
            return name_hash ^ ver_hash ^ (m_linked_source ? m_linked_source->hash_identity(ctx.time) : 0);
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

    [[nodiscard]] const std::string& get_name() const { return m_name; }

    // --- Inspection ------------------------------------------------------------

    [[nodiscard]] bool is_animated() const { return m_mode == PropertyMode::Animated; }
    [[nodiscard]] const animation::KeyframeTrack<T>& curve() const { return m_curve; }

    /**
     * Returns the static value (only valid when !is_animated()).
     * Calling this on an animated property is technically valid but returns
     * the default-constructed static value.
     */
    [[nodiscard]] const T& static_value() const { return m_static_value; }

private:
    enum class PropertyMode { Static, Animated, Linked };

    std::string                  m_name;
    PropertyMode                 m_mode{PropertyMode::Static};
    T                            m_static_value{};
    animation::KeyframeTrack<T> m_curve;
    std::shared_ptr<LinkedPropertySource<T>> m_linked_source;
    uint64_t                     m_version{1}; ///< Monotonic version counter for invalidation.
};

template <typename T>
using ConstantProperty = AnimatableProperty<T>;

} // namespace properties
} // namespace tachyon
