#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include <locale>

namespace tachyon::scene {

std::string trim_copy(const std::string& text);

struct TemplateFormatOptions {
    std::optional<std::string> locale;
};

std::string resolve_template(
    const std::string& tmpl,
    const std::unordered_map<std::string, std::string>* str_vars,
    const std::unordered_map<std::string, double>* num_vars,
    const TemplateFormatOptions& opts = {});

std::string format_number(double value, const std::string& format_spec, const std::string& locale_str = "");
std::string format_date(const std::string& date_str, const std::string& format_spec);

} // namespace tachyon::scene
