#pragma once

#include <string>
#include <optional>
#include <vector>

namespace tachyon {

struct AssetSpec {
    std::string id;
    std::string type;
    std::string path;
    std::string source;
    std::optional<std::string> alpha_mode;
};

struct DataSourceSpec {
    std::string id;
    std::string path;
    std::string type; // "csv", "json"
};

} // namespace tachyon
