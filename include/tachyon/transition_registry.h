#pragma once

#include <string>
#include <functional>

namespace tachyon {

namespace renderer2d {
struct Color;
class SurfaceRGBA;
} // namespace renderer2d

/**
 * TransitionSpec: pure specification for a transition.
 * Contains the ID, metadata, and the function pointer.
 */
struct TransitionSpec {
    using TransitionFn = renderer2d::Color(*)(float u, float v, float t,
                                              const renderer2d::SurfaceRGBA& input,
                                              const renderer2d::SurfaceRGBA* to_surface);

    std::string id;
    std::string name;
    std::string description;
    TransitionFn function{nullptr};
};

/**
 * Runtime registry for transitions.
 * Accessible only via public API functions.
 */
class TransitionRegistry {
public:
    static TransitionRegistry& instance();

    void register_transition(const TransitionSpec& spec);
    void unregister_transition(const std::string& id);

    const TransitionSpec* find(const std::string& id) const;
    std::size_t count() const;
    const TransitionSpec* get_by_index(std::size_t index) const;

private:
    TransitionRegistry();
    struct Impl;
    Impl* m_impl{nullptr};
};

}  // namespace tachyon
