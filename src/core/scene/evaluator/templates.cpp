#include "tachyon/core/scene/evaluator/templates.h"
#include <algorithm>
#include <cstdio>
#include <cstdarg>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cctype>

namespace tachyon::scene {

std::string trim_copy(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

static std::pair<std::string, std::string> parse_template_key(const std::string& key) {
    const auto colon_pos = key.find(':');
    if (colon_pos == std::string::npos) {
        return {key, {}};
    }
    return {trim_copy(key.substr(0, colon_pos)), trim_copy(key.substr(colon_pos + 1))};
}

std::string format_number(double value, const std::string& format_spec, const std::string& locale_str) {
    if (format_spec.empty()) {
        if (value == static_cast<long long>(value)) {
            return std::to_string(static_cast<long long>(value));
        }
        return std::to_string(value);
    }

    // Determine thousand and decimal separators based on locale
    char thousand_sep = ',';
    char decimal_sep = '.';
    
    if (!locale_str.empty()) {
        // Map common locales to their separators
        if (locale_str == "it-IT" || locale_str == "fr-FR" || locale_str == "de-DE" || locale_str == "es-ES") {
            thousand_sep = '.';  // or ' ' for France
            decimal_sep = ',';
        } else if (locale_str == "fr-FR") {
            thousand_sep = ' ';
            decimal_sep = ',';
        }
        // Default for en-US, en-GB, etc. is already set
    }

    std::string result;

    if (format_spec.find('.') != std::string::npos || format_spec.find('f') != std::string::npos) {
        int precision = 2;
        if (format_spec.find('.') != std::string::npos) {
            const auto dot_pos = format_spec.find('.');
            const auto f_pos = format_spec.find('f');
            if (f_pos != std::string::npos && f_pos > dot_pos) {
                const std::string prec_str = format_spec.substr(dot_pos + 1, f_pos - dot_pos - 1);
                if (!prec_str.empty() && std::all_of(prec_str.begin(), prec_str.end(), ::isdigit)) {
                    precision = std::stoi(prec_str);
                }
            }
        }

        char buffer[256];
        if (decimal_sep == ',') {
            // Use comma as decimal separator
            std::snprintf(buffer, sizeof(buffer), "%.*f", precision, value);
            std::string temp = buffer;
            std::replace(temp.begin(), temp.end(), '.', ',');
            result = temp;
        } else {
            std::snprintf(buffer, sizeof(buffer), "%.*f", precision, value);
            result = buffer;
        }

        if (format_spec.find(',') != std::string::npos) {
            const auto dec_pos = result.find(decimal_sep);
            const std::string int_part = (dec_pos != std::string::npos) ? result.substr(0, dec_pos) : result;
            const std::string dec_part = (dec_pos != std::string::npos) ? result.substr(dec_pos) : "";

            std::string formatted_int;
            int count = 0;
            for (auto it = int_part.rbegin(); it != int_part.rend(); ++it) {
                if (count > 0 && count % 3 == 0) {
                    formatted_int += thousand_sep;
                }
                formatted_int += *it;
                count++;
            }
            std::reverse(formatted_int.begin(), formatted_int.end());
            result = formatted_int + dec_part;
        }
    } else {
        bool is_padding = std::all_of(format_spec.begin(), format_spec.end(), [](char c) { return c == '0' || c == '#' || ::isdigit(static_cast<unsigned char>(c)); });
        if (is_padding) {
            int width = 0;
            char pad_char = ' ';
            
            // Check if padding with zeros or spaces
            if (format_spec.find('0') != std::string::npos) {
                pad_char = '0';
            }
            
            // Calculate width: count all digits in the format spec
            for (char c : format_spec) {
                if (::isdigit(static_cast<unsigned char>(c))) {
                    width = width * 10 + (c - '0');
                }
            }
            
            // If width is 0 (e.g., format "000"), use the length of the format spec
            if (width == 0) {
                width = static_cast<int>(format_spec.length());
            }
            
            long long int_val = static_cast<long long>(value);
            std::string num_str = std::to_string(int_val);
            if (static_cast<int>(num_str.size()) < width) {
                result = std::string(width - num_str.size(), pad_char) + num_str;
            } else {
                result = num_str;
            }
        } else {
            if (value == static_cast<long long>(value)) {
                result = std::to_string(static_cast<long long>(value));
            } else {
                result = std::to_string(value);
            }
        }
    }

    return result;
}

std::string format_date(const std::string& date_str, const std::string& format_spec) {
    if (format_spec.empty()) {
        return date_str;
    }

    std::time_t time = 0;
    if (date_str.empty()) {
        time = std::time(nullptr);
    } else {
        std::tm tm = {};
        std::istringstream ss(date_str);
        
        // Try parsing as "%Y-%m-%d %H:%M:%S"
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        if (ss.fail()) {
            ss.clear();
            ss.str(date_str);
            // Try parsing as "%Y-%m-%d"
            ss >> std::get_time(&tm, "%Y-%m-%d");
            if (ss.fail()) {
                // Default to current time
                time = std::time(nullptr);
                std::tm local_tm;
#ifdef _WIN32
                localtime_s(&local_tm, &time);
#else
                localtime_r(&time, &local_tm);
#endif
                char buffer[256];
                std::strftime(buffer, sizeof(buffer), format_spec.c_str(), &local_tm);
                return buffer;
            }
        }
        time = std::mktime(&tm);
    }

    std::tm local_tm;
#ifdef _WIN32
    localtime_s(&local_tm, &time);
#else
    localtime_r(&time, &local_tm);
#endif

    char buffer[256];
    std::strftime(buffer, sizeof(buffer), format_spec.c_str(), &local_tm);
    return buffer;
}

std::string resolve_template(
    const std::string& tmpl,
    const std::unordered_map<std::string, std::string>* str_vars,
    const std::unordered_map<std::string, double>* num_vars,
    const TemplateFormatOptions& opts) {
    (void)opts;

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

        const std::string full_key = trim_copy(tmpl.substr(open + 2, close - (open + 2)));
        const auto [key, format_spec] = parse_template_key(full_key);

        bool found = false;
        if (str_vars) {
            const auto sit = str_vars->find(key);
            if (sit != str_vars->end()) {
                if (!format_spec.empty() && (key == "date" || key == "today" || key == "time")) {
                    result += format_date(sit->second, format_spec);
                } else {
                    result += sit->second;
                }
                found = true;
            }
        }
        if (!found && num_vars) {
            const auto nit = num_vars->find(key);
            if (nit != num_vars->end()) {
                result += format_number(nit->second, format_spec, opts.locale.value_or(""));
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
