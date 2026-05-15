#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <memory>

namespace tachyon::core::json {

/**
 * @brief Represents a JSON value (object, array, string, number, bool, null).
 * 
 * This is a lightweight handle to a value within a JsonDocument.
 */
class JsonValue {
public:
    JsonValue() = default;

    /**
     * @brief Check if the value is valid.
     */
    bool valid() const noexcept;

    /**
     * @brief Get a string value by key (for objects).
     */
    std::optional<std::string> get_string(std::string_view key) const;

    /**
     * @brief Get a double value by key (for objects).
     */
    std::optional<double> get_double(std::string_view key) const;

    /**
     * @brief Get an integer value by key (for objects).
     */
    std::optional<std::int64_t> get_int(std::string_view key) const;

    /**
     * @brief Get a boolean value by key (for objects).
     */
    std::optional<bool> get_bool(std::string_view key) const;

    /**
     * @brief Get a string array value by key (for objects).
     */
    std::optional<std::vector<std::string>> get_string_array(std::string_view key) const;

private:
    friend class JsonDocument;
    struct Impl;
    std::shared_ptr<Impl> impl_;
};

/**
 * @brief Represents a parsed JSON document.
 * 
 * This class owns the memory buffer and the parsed DOM.
 */
class JsonDocument {
public:
    JsonDocument();
    ~JsonDocument();

    JsonDocument(JsonDocument&&) noexcept;
    JsonDocument& operator=(JsonDocument&&) noexcept;

    JsonDocument(const JsonDocument&) = delete;
    JsonDocument& operator=(const JsonDocument&) = delete;

    /**
     * @brief Parse a JSON file from disk.
     * @param path Path to the JSON file.
     * @param error Optional pointer to receive error message.
     * @return true if successful, false otherwise.
     */
    bool parse_file(const std::string& path, std::string* error = nullptr);

    /**
     * @brief Parse a JSON string from memory.
     * @param json JSON string view.
     * @param error Optional pointer to receive error message.
     * @return true if successful, false otherwise.
     */
    bool parse_string(std::string_view json, std::string* error = nullptr);

    /**
     * @brief Get the root value of the document.
     */
    JsonValue root() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace tachyon::core::json
