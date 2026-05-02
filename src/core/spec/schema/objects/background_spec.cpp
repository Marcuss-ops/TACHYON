#include "tachyon/core/spec/schema/objects/background_spec.h"
#include <algorithm>
#include <cstdlib>
#include <string>
#include <nlohmann/json.hpp>

namespace tachyon {

BackgroundSpec BackgroundSpec::from_string(const std::string& str) {
    BackgroundSpec result;
    result.value = str;

    // Try to detect if it's a color
    // Hex color: #RGB, #RGBA, #RRGGBB, #RRGGBBAA
    if (str.starts_with('#')) {
        std::string hex = str.substr(1);
        // Expand short hex
        if (hex.length() == 3 || hex.length() == 4) {
            char r = hex[0], g = hex[1], b = hex[2];
            char a = hex.length() == 4 ? hex[3] : 'F';
            hex = {r, r, g, g, b, b, a, a};
        }
        if (hex.length() == 6 || hex.length() == 8) {
            try {
                uint32_t val = std::stoul(hex, nullptr, 16);
                result.type = BackgroundType::Color;
                ColorSpec color;
                if (hex.length() == 6) {
                    color = {
                        static_cast<std::uint8_t>((val >> 16) & 0xFF),
                        static_cast<std::uint8_t>((val >> 8) & 0xFF),
                        static_cast<std::uint8_t>(val & 0xFF),
                        255
                    };
                } else {
                    color = {
                        static_cast<std::uint8_t>((val >> 24) & 0xFF),
                        static_cast<std::uint8_t>((val >> 16) & 0xFF),
                        static_cast<std::uint8_t>((val >> 8) & 0xFF),
                        static_cast<std::uint8_t>(val & 0xFF)
                    };
                }
                result.parsed_color = color;
                return result;
            } catch (...) {}
        }
    }

    // RGB(A) syntax: rgb(r,g,b) or rgba(r,g,b,a)
    if (str.starts_with("rgb(") || str.starts_with("rgba(")) {
        result.type = BackgroundType::Color;
        result.parsed_color = ColorSpec{0, 0, 0, 255}; // Default
        // Simple parser for basic case
        size_t start = str.find('(');
        size_t end = str.find(')');
        if (start != std::string::npos && end != std::string::npos) {
            std::string params = str.substr(start + 1, end - start - 1);
            std::replace(params.begin(), params.end(), ',', ' ');
            std::stringstream ss(params);
            std::vector<float> vals;
            std::string token;
            while (ss >> token) {
                if (token.back() == '%') {
                    token.pop_back();
                    try {
                        float pct = std::stof(token);
                        vals.push_back(pct / 100.0f * 255.0f);
                    } catch (...) {}
                } else {
                    try {
                        float v = std::stof(token);
                        vals.push_back(v);
                    } catch (...) {}
                }
            }
            if (vals.size() >= 3) {
                const std::uint8_t alpha = vals.size() >= 4
                    ? static_cast<std::uint8_t>(std::clamp(vals[3], 0.0f, 255.0f))
                    : 255;
                result.parsed_color = ColorSpec{
                    static_cast<std::uint8_t>(std::clamp(vals[0], 0.0f, 255.0f)),
                    static_cast<std::uint8_t>(std::clamp(vals[1], 0.0f, 255.0f)),
                    static_cast<std::uint8_t>(std::clamp(vals[2], 0.0f, 255.0f)),
                    alpha
                };
            }
        }
        return result;
    }

    // Basic color names
    static const std::unordered_map<std::string, ColorSpec> color_names = {
        {"black", {0, 0, 0, 255}},
        {"white", {255, 255, 255, 255}},
        {"red", {255, 0, 0, 255}},
        {"green", {0, 255, 0, 255}},
        {"blue", {0, 0, 255, 255}},
        {"transparent", {0, 0, 0, 0}}
    };

    auto it = color_names.find(str);
    if (it != color_names.end()) {
        result.type = BackgroundType::Color;
        result.parsed_color = it->second;
        return result;
    }

    // Check if it looks like a component ID (starts with "component:" or common component prefixes)
    if (str.starts_with("component:")) {
        result.type = BackgroundType::Component;
        result.value = str.substr(10); // Remove "component:" prefix
        return result;
    }

    // Check if it looks like an asset reference (contains '/' or ends with common image extensions)
    if (str.find('/') != std::string::npos || str.ends_with(".png") || str.ends_with(".jpg") || str.ends_with(".jpeg")) {
        result.type = BackgroundType::Asset;
        return result;
    }

    // Default: treat as component ID (most common use case for background)
    result.type = BackgroundType::Component;
    return result;
}

void to_json(nlohmann::json& j, const BackgroundSpec& spec) {
    j = nlohmann::json{
        {"type", spec.type == BackgroundType::Color ? "color" :
                spec.type == BackgroundType::Component ? "component" :
                spec.type == BackgroundType::Asset ? "asset" : "preset"},
        {"value", spec.value}
    };
    if (spec.parsed_color.has_value()) {
        const auto& c = *spec.parsed_color;
        char buf[16];
        std::snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", c.r, c.g, c.b, c.a);
        j["parsed_color"] = buf;
    }
}

void from_json(const nlohmann::json& j, BackgroundSpec& spec) {
    if (j.is_string()) {
        // Backward compatible: plain string
        spec = BackgroundSpec::from_string(j.get<std::string>());
        return;
    }

    if (j.is_object()) {
        std::string type_str = j.value("type", "component");
        if (type_str == "color") {
            spec.type = BackgroundType::Color;
        } else if (type_str == "asset") {
            spec.type = BackgroundType::Asset;
        } else if (type_str == "preset") {
            spec.type = BackgroundType::Preset;
        } else {
            spec.type = BackgroundType::Component;
        }
        spec.value = j.value("value", "");

        if (j.contains("parsed_color") && j["parsed_color"].is_string()) {
            spec.parsed_color = BackgroundSpec::from_string(j["parsed_color"].get<std::string>()).parsed_color;
        } else {
            if (spec.type == BackgroundType::Color) {
                spec.parsed_color = BackgroundSpec::from_string(spec.value).parsed_color;
            }
        }
    }
}

} // namespace tachyon
