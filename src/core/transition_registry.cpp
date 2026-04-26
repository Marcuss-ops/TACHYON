#include "tachyon/transition_registry.h"
#include "tachyon/renderer2d/core/framebuffer.h"

#include <unordered_map>
#include <memory>
#include <algorithm>

namespace tachyon {

struct TransitionRegistry::Impl {
    std::vector<TransitionSpec> transitions;
    std::unordered_map<std::string, std::size_t> id_to_index;
};

TransitionRegistry& TransitionRegistry::instance() {
    static TransitionRegistry inst;
    return inst;
}

TransitionRegistry::TransitionRegistry() : m_impl(new Impl) {}

void TransitionRegistry::register_transition(const TransitionSpec& spec) {
    auto it = m_impl->id_to_index.find(spec.id);
    if (it != m_impl->id_to_index.end()) {
        m_impl->transitions[it->second] = spec;
    } else {
        m_impl->id_to_index[spec.id] = m_impl->transitions.size();
        m_impl->transitions.push_back(spec);
    }
}

void TransitionRegistry::unregister_transition(const std::string& id) {
    auto it = m_impl->id_to_index.find(id);
    if (it != m_impl->id_to_index.end()) {
        const std::size_t index = it->second;
        m_impl->transitions.erase(m_impl->transitions.begin() + index);
        m_impl->id_to_index.erase(it);
        // Rebuild indices
        m_impl->id_to_index.clear();
        for (std::size_t i = 0; i < m_impl->transitions.size(); ++i) {
            m_impl->id_to_index[m_impl->transitions[i].id] = i;
        }
    }
}

const TransitionSpec* TransitionRegistry::find(const std::string& id) const {
    auto it = m_impl->id_to_index.find(id);
    if (it != m_impl->id_to_index.end()) {
        return &m_impl->transitions[it->second];
    }
    return nullptr;
}

std::size_t TransitionRegistry::count() const {
    return m_impl->transitions.size();
}

const TransitionSpec* TransitionRegistry::get_by_index(std::size_t index) const {
    if (index >= m_impl->transitions.size()) return nullptr;
    return &m_impl->transitions[index];
}

}  // namespace tachyon
