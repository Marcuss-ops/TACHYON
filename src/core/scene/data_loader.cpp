#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <stdexcept>

namespace tachyon::scene {

std::vector<std::vector<std::string>> load_csv(const std::string& path) {
    std::vector<std::vector<std::string>> table;
    std::ifstream file(path);
    if (!file.is_open()) return table;

    std::string line;
    while (std::getline(file, line)) {
        std::vector<std::string> row;
        std::stringstream ss(line);
        std::string cell;
        bool in_quotes = false;
        std::string current_cell;
        
        for (char ch : line) {
            if (ch == '"') {
                in_quotes = !in_quotes;
            } else if (ch == ',' && !in_quotes) {
                row.push_back(current_cell);
                current_cell.clear();
            } else {
                current_cell += ch;
            }
        }
        row.push_back(current_cell);
        table.push_back(std::move(row));
    }
    return table;
}

// Simple loader registry
void load_data_sources(const SceneSpec& scene, std::unordered_map<std::string, std::vector<std::vector<std::string>>>& tables) {
    for (const auto& ds : scene.data_sources) {
        if (ds.type == "csv") {
            tables[ds.id] = load_csv(ds.path);
        }
    }
}

} // namespace tachyon::scene
