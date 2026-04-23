#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>

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

std::vector<std::vector<std::string>> load_json_table(const std::string& path) {
    std::vector<std::vector<std::string>> table;
    std::ifstream file(path);
    if (!file.is_open()) return table;

    try {
        nlohmann::json j;
        file >> j;
        if (j.is_array()) {
            for (const auto& item : j) {
                std::vector<std::string> row;
                if (item.is_array()) {
                    for (const auto& val : item) {
                        row.push_back(val.is_string() ? val.get<std::string>() : val.dump());
                    }
                } else if (item.is_object()) {
                    for (auto it = item.begin(); it != item.end(); ++it) {
                        row.push_back(it.value().is_string() ? it.value().get<std::string>() : it.value().dump());
                    }
                }
                table.push_back(std::move(row));
            }
        }
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Failed to parse JSON table at " + path + ": " + e.what());
    } catch (const std::exception& e) {
        throw std::runtime_error("Error loading JSON table at " + path + ": " + e.what());
    } catch (...) {
        throw std::runtime_error("Unknown error loading JSON table at " + path);
    }
    return table;
}

// Simple loader registry
void load_data_sources(const SceneSpec& scene, std::unordered_map<std::string, std::vector<std::vector<std::string>>>& tables) {
    for (const auto& ds : scene.data_sources) {
        if (ds.type == "csv") {
            tables[ds.id] = load_csv(ds.path);
        } else if (ds.type == "json") {
            tables[ds.id] = load_json_table(ds.path);
        }
    }
}

} // namespace tachyon::scene
