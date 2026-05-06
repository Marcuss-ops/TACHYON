#include "tachyon/transition_registry.h"
#include "tachyon/renderer2d/core/framebuffer.h"

#include <memory>
#include <unordered_map>
#include <utility>

namespace tachyon {

struct TransitionRegistry::Impl {
    registry::TypedRegistry<TransitionSpec> transitions;
    std::unordered_map<std::string, TransitionSpec::TransitionFn> cpu_implementations;
};

TransitionRegistry& TransitionRegistry::instance() {
    static TransitionRegistry inst;
    return inst;
}

TransitionRegistry::TransitionRegistry() : m_impl(std::make_unique<Impl>()) {}
TransitionRegistry::~TransitionRegistry() = default;

void TransitionRegistry::register_transition(const TransitionSpec& spec) {
    if (spec.id.empty()) {
        return;
    }

    m_impl->transitions.register_spec(spec);
}

void TransitionRegistry::unregister_transition(const std::string& id) {
    (void)m_impl->transitions.erase(id);
}

const TransitionSpec* TransitionRegistry::find(const std::string& id) const {
    return m_impl->transitions.find(id);
}

std::size_t TransitionRegistry::count() const {
    return m_impl->transitions.list_ids().size();
}

const TransitionSpec* TransitionRegistry::get_by_index(std::size_t index) const {
    const auto ids = m_impl->transitions.list_ids();
    if (index >= ids.size()) {
        return nullptr;
    }
    return m_impl->transitions.find(ids[index]);
}

std::vector<std::string> TransitionRegistry::list_builtin_transition_ids() const {
    return m_impl->transitions.list_ids();
}

std::vector<TransitionRegistry::TransitionInfo> TransitionRegistry::list_builtin_transitions() const {
    std::vector<TransitionInfo> infos;
    const auto ids = m_impl->transitions.list_ids();
    infos.reserve(ids.size());
    for (const auto& id : ids) {
        const auto* spec = m_impl->transitions.find(id);
        if (spec == nullptr) {
            continue;
        }
        infos.push_back({
            id,
            spec->name,
            spec->description,
            spec->function != nullptr,
            spec->state_type != TransitionSpec::Type::None
        });
    }
    return infos;
}

void TransitionRegistry::register_cpu_implementation(const std::string& name, TransitionSpec::TransitionFn fn) {
    m_impl->cpu_implementations[name] = fn;
    
    // Auto-update any already registered specs that were waiting for this implementation
    const auto ids = m_impl->transitions.list_ids();
    for (const auto& id : ids) {
        auto* spec = const_cast<TransitionSpec*>(m_impl->transitions.find(id));
        if (spec && spec->function == nullptr && spec->cpu_fn_name == name) {
            spec->function = fn;
        }
    }
}

TransitionSpec::TransitionFn TransitionRegistry::find_cpu_implementation(const std::string& name) const {
    const auto it = m_impl->cpu_implementations.find(name);
    if (it != m_impl->cpu_implementations.end()) {
        return it->second;
    }
    return nullptr;
}

}  // namespace tachyon
