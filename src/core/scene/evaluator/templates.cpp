#include "tachyon/core/scene/evaluator/templates.h"
#include <algorithm>

namespace tachyon::scene {

std::string trim_copy(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string resolve_template(
    const std::string& tmpl,
    const std::unordered_map<std::string, std::string>* str_vars,
    const std::unordered_map<std::string, double>* num_vars) {
    if (tmpl.find("{{") == std::string::npos) return tmpl;

    std::string result;
    result.reserve(tmpl.size());
    std::size_t cursor = 0;
    while (cursor < tmpl.size()) {
        const std::size_t open = tmpl.find("{{", cursor);
        if (open == std::string::npos) {
            result.append(tmpl, cursor, std::string::npos);
            break;
        }

        result.append(tmpl, cursor, open - cursor);
        const std::size_t close = tmpl.find("}}", open + 2);
        if (close == std::string::npos) {
            result.append(tmpl, open, std::string::npos);
            break;
        }

        const std::string key = trim_copy(tmpl.substr(open + 2, close - (open + 2)));
        bool found = false;
        if (str_vars) {
            const auto sit = str_vars->find(key);
            if (sit != str_vars->end()) {
                result += sit->second;
                found = true;
            }
        }
        if (!found && num_vars) {
            const auto nit = num_vars->find(key);
            if (nit != num_vars->end()) {
                auto d = nit->second;
                if (d == static_cast<long long>(d)) {
                    result += std::to_string(static_cast<long long>(d));
                } else {
                    result += std::to_string(d);
                }
                found = true;
            }
        }

        if (!found) {
            result.append(tmpl, open, close - open + 2);
        }
        cursor = close + 2;
    }
    return result;
}

} // namespace tachyon::scene
