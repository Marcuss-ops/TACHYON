#include "tachyon/transition_registry.h"
#include "tachyon/renderer2d/core/framebuffer.h"

#include <unordered_map>
#include <memory>
#include <algorithm>

namespace tachyon {

struct TransitionRegistry::Impl {
    std::unordered_map<std::string, TransitionSpec> transitions;
};

TransitionRegistry& TransitionRegistry::instance() {
    static TransitionRegistry inst;
    return inst;
}

TransitionRegistry::TransitionRegistry() : m_impl(new Impl) {}

void TransitionRegistry::register_transition(const TransitionSpec& spec) {
    m_impl->transitions[spec.id] = spec;
}

void TransitionRegistry::unregister_transition(const std::string& id) {
    m_impl->transitions.erase(id);
}

const TransitionSpec* TransitionRegistry::find(const std::string& id) const {
    auto it = m_impl->transitions.find(id);
    if (it != m_impl->transitions.end()) {
        return &it->second;
    }
    return nullptr;
}

std::size_t TransitionRegistry::count() const {
    return m_impl->transitions.size();
}

const TransitionSpec* TransitionRegistry::get_by_index(std::size_t index) const {
    if (index >= m_impl->transitions.size()) return nullptr;
    auto it = m_impl->transitions.begin();
    std::advance(it, index);
    return &it->second;
}

}  // namespace tachyon
