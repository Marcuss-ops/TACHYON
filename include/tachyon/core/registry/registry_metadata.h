#pragma once

#include <string>
#include <vector>

namespace tachyon::registry {

struct RegistryMetadata {
    std::string id;
    std::string display_name;
    std::string description;
    std::string category;
    std::vector<std::string> tags;
    int version{1};
    bool stable{true};
};

} // namespace tachyon::registry
