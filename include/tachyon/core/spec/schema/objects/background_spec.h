#pragma once

#include "tachyon/core/api.h"
#include "tachyon/core/types/colors.h"
#include <string>
#include <optional>

namespace tachyon {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

/// Represents the type of background specified
enum class BackgroundType {
    None,       // No background
    Color,      // Background is a color (hex, rgb, etc.)
    Component,  // Background references a component by ID
    Asset,      // Background references an asset by name/path
    Preset      // Background references a preset by name
};

/// A well-typed background specification that replaces the ambiguous std::optional<std::string>.
struct BackgroundSpec {
    BackgroundType type{BackgroundType::Color};
    std::string value;  // Color string, component ID, asset name, or preset name

    /// For Color type: the parsed color (valid only when type == Color)
    std::optional<ColorSpec> parsed_color;

    /// Try to parse a string as a background specification.
    /// @returns a BackgroundSpec with the appropriate type detected.
    TACHYON_API static BackgroundSpec from_string(const std::string& str);

    BackgroundSpec() = default;
    explicit BackgroundSpec(const std::string& str) : BackgroundSpec(from_string(str)) {}

    /// Check if this background is a color that can be used for frame clear.
    bool is_color() const { return type == BackgroundType::Color; }

    /// Check if this background references a component.
    bool is_component() const { return type == BackgroundType::Component; }

    /// Get the color value (only valid when is_color() is true).
    std::optional<ColorSpec> get_color() const {
        if (type == BackgroundType::Color && parsed_color.has_value()) {
            return parsed_color;
        }
        return std::nullopt;
    }

    [[nodiscard]] bool operator==(const BackgroundSpec& other) const {
        return type == other.type && value == other.value && parsed_color == other.parsed_color;
    }
};

} // namespace tachyon

#ifdef _MSC_VER
#pragma warning(pop)
#endif
