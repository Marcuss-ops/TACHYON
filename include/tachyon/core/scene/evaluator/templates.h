#pragma once

#include <string>
#include <unordered_map>

namespace tachyon::scene {

std::string trim_copy(const std::string& text);
std::string resolve_template(
    const std::string& tmpl,
    const std::unordered_map<std::string, std::string>* str_vars,
    const std::unordered_map<std::string, double>* num_vars);

} // namespace tachyon::scene
