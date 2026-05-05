#include "tachyon/transition_registry.h"
#include "tachyon/renderer2d/core/framebuffer.h"

#include <memory>
#include <utility>

namespace tachyon {

struct TransitionRegistry::Impl {
    registry::TypedRegistry<TransitionSpec> transitions;
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

}  // namespace tachyon
