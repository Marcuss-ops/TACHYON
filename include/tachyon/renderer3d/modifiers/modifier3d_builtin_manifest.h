#pragma once

#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/registry/parameter_schema.h"
#include "tachyon/renderer3d/modifiers/modifier3d_registry.h"

#include <string>
#include <utility>
#include <vector>

namespace tachyon::renderer3d {

struct Modifier3DBuiltinSpec {
    std::string id;
    std::string display_name;
    std::string description;
    std::string category;
    std::vector<std::string> tags;
    registry::ParameterSchema schema;
    Modifier3DDescriptor::Factory factory;
};

template <typename T>
Modifier3DDescriptor::Factory make_modifier_factory() {
    return []() {
        return std::make_unique<T>();
    };
}

inline Modifier3DDescriptor make_modifier_descriptor(Modifier3DBuiltinSpec spec) {
    Modifier3DDescriptor desc;
    desc.id = std::move(spec.id);
    desc.metadata = {
        desc.id,
        std::move(spec.display_name),
        std::move(spec.description),
        std::move(spec.category),
        std::move(spec.tags)
    };
    desc.schema = std::move(spec.schema);
    desc.factory = std::move(spec.factory);
    return desc;
}

inline void register_modifier_builtin(Modifier3DRegistry& registry, Modifier3DBuiltinSpec spec) {
    registry.register_spec(make_modifier_descriptor(std::move(spec)));
}

} // namespace tachyon::renderer3d
