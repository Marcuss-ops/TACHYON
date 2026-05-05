#pragma once

#include <string>
#include <vector>

namespace tachyon::registry {

struct Capabilities {
    std::vector<std::string> works_on; // text, image, video, 3d, etc.
    std::vector<std::string> requirements; // gpu, depth, alpha, etc.
};

enum class Stability {
    Experimental,
    Stable,
    Deprecated
};

struct RegistryMetadata {
    std::string id;
    std::string display_name;
    std::string description;
    std::string category;
    std::vector<std::string> tags;
    int version{1};
    Stability stability{Stability::Stable};
    Capabilities capabilities;
};

} // namespace tachyon::registry
