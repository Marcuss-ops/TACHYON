#include "tachyon/core/json/json_document.h"

#if defined(TACHYON_ENABLE_SIMDJSON)
#include <simdjson.h>
#endif

#include <fstream>
#include <sstream>
#include <filesystem>

namespace tachyon::core::json {

struct JsonDocument::Impl {
#if defined(TACHYON_ENABLE_SIMDJSON)
    simdjson::dom::parser parser;
    simdjson::dom::element root;
#endif
    std::string buffer;
    bool valid = false;
};

// JsonValue implementation (skeleton)
bool JsonValue::valid() const noexcept { return false; }
std::optional<std::string> JsonValue::get_string(std::string_view) const { return std::nullopt; }
std::optional<double> JsonValue::get_double(std::string_view) const { return std::nullopt; }
std::optional<std::int64_t> JsonValue::get_int(std::string_view) const { return std::nullopt; }
std::optional<bool> JsonValue::get_bool(std::string_view) const { return std::nullopt; }

JsonDocument::JsonDocument()
    : impl_(std::make_unique<Impl>()) {}

JsonDocument::~JsonDocument() = default;

JsonDocument::JsonDocument(JsonDocument&& other) noexcept = default;
JsonDocument& JsonDocument::operator=(JsonDocument&& other) noexcept = default;

bool JsonDocument::parse_file(const std::string& path, std::string* error) {
    if (!std::filesystem::exists(path)) {
        if (error) *error = "File does not exist: " + path;
        return false;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        if (error) *error = "Cannot open JSON file: " + path;
        return false;
    }

    std::size_t file_size = std::filesystem::file_size(path);
    impl_->buffer.resize(file_size);
    file.read(impl_->buffer.data(), file_size);
    
    return parse_string(impl_->buffer, error);
}

bool JsonDocument::parse_string(std::string_view json, std::string* error) {
#if defined(TACHYON_ENABLE_SIMDJSON)
    // If not using the internal buffer from parse_file, we must copy it
    // because simdjson needs a padded buffer or we need to manage lifetime.
    if (impl_->buffer.data() != json.data()) {
        impl_->buffer.assign(json.data(), json.size());
    }

    auto result = impl_->parser.parse(impl_->buffer);
    if (result.error()) {
        if (error) *error = simdjson::error_message(result.error());
        impl_->valid = false;
        return false;
    }

    impl_->root = result.value();
    impl_->valid = true;
    return true;
#else
    if (error) *error = "simdjson disabled";
    impl_->valid = false;
    return false;
#endif
}

JsonValue JsonDocument::root() const {
    return JsonValue();
}

} // namespace tachyon::core::json
