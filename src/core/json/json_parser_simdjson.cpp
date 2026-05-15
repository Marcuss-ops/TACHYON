#include <fstream>
#include <sstream>
#include <filesystem>
#include <stdexcept>
#include <system_error>

#include "tachyon/core/json/json_document.h"

#if defined(TACHYON_ENABLE_SIMDJSON)
#include <simdjson.h>
#endif

namespace tachyon::core::json {

struct JsonValue::Impl {
#if defined(TACHYON_ENABLE_SIMDJSON)
    simdjson::dom::element element;
#endif
    bool valid = false;
};

bool JsonValue::valid() const noexcept { return impl_ && impl_->valid; }

std::optional<std::string> JsonValue::get_string(std::string_view key) const {
#if defined(TACHYON_ENABLE_SIMDJSON)
    if (!valid()) return std::nullopt;
    auto val = impl_->element[key];
    if (val.error()) return std::nullopt;
    std::string_view sv;
    if (val.get(sv) == simdjson::SUCCESS) return std::string(sv);
#endif
    return std::nullopt;
}

std::optional<double> JsonValue::get_double(std::string_view key) const {
#if defined(TACHYON_ENABLE_SIMDJSON)
    if (!valid()) return std::nullopt;
    auto val = impl_->element[key];
    if (val.error()) return std::nullopt;
    double d;
    if (val.get(d) == simdjson::SUCCESS) return d;
#endif
    return std::nullopt;
}

std::optional<std::int64_t> JsonValue::get_int(std::string_view key) const {
#if defined(TACHYON_ENABLE_SIMDJSON)
    if (!valid()) return std::nullopt;
    auto val = impl_->element[key];
    if (val.error()) return std::nullopt;
    std::int64_t i;
    if (val.get(i) == simdjson::SUCCESS) return i;
#endif
    return std::nullopt;
}

std::optional<bool> JsonValue::get_bool(std::string_view key) const {
#if defined(TACHYON_ENABLE_SIMDJSON)
    if (!valid()) return std::nullopt;
    auto val = impl_->element[key];
    if (val.error()) return std::nullopt;
    bool b;
    if (val.get(b) == simdjson::SUCCESS) return b;
#endif
    return std::nullopt;
}

std::optional<std::vector<std::string>> JsonValue::get_string_array(std::string_view key) const {
#if defined(TACHYON_ENABLE_SIMDJSON)
    if (!valid()) return std::nullopt;
    auto val = impl_->element[key];
    if (val.error() || !val.is_array()) return std::nullopt;
    std::vector<std::string> result;
    for (auto item : val.get_array()) {
        std::string_view sv;
        if (item.get(sv) == simdjson::SUCCESS) result.push_back(std::string(sv));
    }
    return result;
#endif
    return std::nullopt;
}

struct JsonDocument::Impl {
#if defined(TACHYON_ENABLE_SIMDJSON)
    simdjson::dom::parser parser;
    simdjson::dom::element root;
#endif
    std::string buffer;
    bool valid = false;
};

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
    impl_->buffer.resize(file_size + simdjson::SIMDJSON_PADDING);
    file.read(impl_->buffer.data(), file_size);
    
    return parse_string(std::string_view(impl_->buffer.data(), file_size), error);
}

bool JsonDocument::parse_string(std::string_view json, std::string* error) {
#if defined(TACHYON_ENABLE_SIMDJSON)
    // simdjson needs padding. If the input view doesn't have it (likely),
    // we copy to our internal buffer which has it.
    if (impl_->buffer.data() != json.data()) {
        impl_->buffer.assign(json.data(), json.size());
        impl_->buffer.resize(json.size() + simdjson::SIMDJSON_PADDING);
    }

    auto result = impl_->parser.parse(impl_->buffer.data(), json.size());
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
    JsonValue v;
    if (impl_->valid) {
        v.impl_ = std::make_shared<JsonValue::Impl>();
#if defined(TACHYON_ENABLE_SIMDJSON)
        v.impl_->element = impl_->root;
#endif
        v.impl_->valid = true;
    }
    return v;
}

} // namespace tachyon::core::json
