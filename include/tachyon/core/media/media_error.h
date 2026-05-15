#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <utility>

namespace tachyon::core {

/**
 * @brief Machine-readable error codes for the media domain.
 * Derived from ruststream-core/core/errors.rs
 */
enum class MediaErrorCode {
    Decode,
    Encode,
    Effects,
    Overlay,
    Audio,
    Timeline,
    Pipeline,
    Init,
    IO,
    Cache,
    Unknown
};

/**
 * @brief Converts a MediaErrorCode to its canonical string representation.
 */
inline std::string_view to_string(MediaErrorCode code) {
    switch (code) {
        case MediaErrorCode::Decode: return "decode";
        case MediaErrorCode::Encode: return "encode";
        case MediaErrorCode::Effects: return "effects";
        case MediaErrorCode::Overlay: return "overlay";
        case MediaErrorCode::Audio: return "audio";
        case MediaErrorCode::Timeline: return "timeline";
        case MediaErrorCode::Pipeline: return "pipeline";
        case MediaErrorCode::Init: return "init";
        case MediaErrorCode::IO: return "io";
        case MediaErrorCode::Cache: return "cache";
        default: return "unknown";
    }
}

/**
 * @brief Structured error for the media pipeline.
 * Supports fluent annotation of stage, path, and fallback status.
 */
class MediaError {
public:
    MediaErrorCode code;
    std::string message;
    std::optional<std::string> stage;
    std::optional<std::string> path;
    bool fallback_used{false};

    MediaError(MediaErrorCode code, std::string message)
        : code(code), message(std::move(message)) {}

    /**
     * @brief Annotates the error with the pipeline stage where it occurred.
     */
    MediaError& with_stage(std::string stage_name) {
        stage = std::move(stage_name);
        return *this;
    }

    /**
     * @brief Annotates the error with the file path involved in the failure.
     */
    MediaError& with_path(std::string file_path) {
        path = std::move(file_path);
        return *this;
    }

    /**
     * @brief Marks whether an emergency fallback was used as a result of this error.
     */
    MediaError& with_fallback(bool used = true) {
        fallback_used = used;
        return *this;
    }

    /**
     * @brief Returns a human-readable representation of the error with all context.
     */
    [[nodiscard]] std::string to_diagnostic_string() const {
        std::string result = std::string(tachyon::core::to_string(code)) + ": " + message;
        if (stage) result += " [stage: " + *stage + "]";
        if (path) result += " [path: " + *path + "]";
        if (fallback_used) result += " (fallback used)";
        return result;
    }
};

/**
 * @brief Result type for media operations.
 */
template <typename T>
struct MediaResult {
    std::optional<T> value;
    std::optional<MediaError> error;

    static MediaResult success(T val) {
        return {std::move(val), std::nullopt};
    }

    static MediaResult failure(MediaError err) {
        return {std::nullopt, std::move(err)};
    }

    [[nodiscard]] bool ok() const noexcept {
        return value.has_value();
    }

    [[nodiscard]] T& operator*() { return *value; }
    [[nodiscard]] const T& operator*() const { return *value; }
    [[nodiscard]] T* operator->() { return &*value; }
    [[nodiscard]] const T* operator->() const { return &*value; }
};

// Specialized MediaResult for void operations
template <>
struct MediaResult<void> {
    bool is_ok{true};
    std::optional<MediaError> error;

    static MediaResult success() {
        return {true, std::nullopt};
    }

    static MediaResult failure(MediaError err) {
        return {false, std::move(err)};
    }

    [[nodiscard]] bool ok() const noexcept {
        return is_ok;
    }
};

} // namespace tachyon::core
