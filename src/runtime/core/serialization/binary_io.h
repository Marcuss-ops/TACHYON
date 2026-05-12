#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <algorithm>
#include <stdexcept>

namespace tachyon::runtime {

// Limits for safety
constexpr std::uint32_t kMaxCompositions = 1024;
constexpr std::uint32_t kMaxLayers = 100000;
constexpr std::uint32_t kMaxStringBytes = 16 * 1024 * 1024;
constexpr std::uint32_t kMaxVectorItems = 10'000'000;

struct BinaryWriter {
    std::vector<std::uint8_t> buffer;

    void write_u8(std::uint8_t value) { write_raw(value); }
    void write_u16(std::uint16_t value) { write_raw(value); }
    void write_u32(std::uint32_t value) { write_raw(value); }
    void write_u64(std::uint64_t value) { write_raw(value); }
    void write_f32(float value) { write_raw(value); }
    void write_f64(double value) { write_raw(value); }
    void write_bool(bool value) { write_u8(value ? 1 : 0); }

    void write_string(const std::string& str) {
        std::uint32_t size = static_cast<std::uint32_t>(std::min<std::size_t>(str.size(), kMaxStringBytes));
        write_u32(size);
        buffer.insert(buffer.end(), str.begin(), str.begin() + size);
    }

    template<typename T, typename Serializer>
    void write_vector(const std::vector<T>& vec, Serializer serializer) {
        std::uint32_t size = static_cast<std::uint32_t>(std::min<std::size_t>(vec.size(), kMaxVectorItems));
        write_u32(size);
        for (std::uint32_t i = 0; i < size; ++i) {
            serializer(*this, vec[i]);
        }
    }

    template<typename T>
    void write_vector(const std::vector<T>& vec) {
        std::uint32_t size = static_cast<std::uint32_t>(std::min<std::size_t>(vec.size(), kMaxVectorItems));
        write_u32(size);
        if constexpr (std::is_trivially_copyable_v<T>) {
            const auto* ptr = reinterpret_cast<const std::uint8_t*>(vec.data());
            buffer.insert(buffer.end(), ptr, ptr + (static_cast<std::size_t>(size) * sizeof(T)));
        }
    }

    template<typename T>
    void write(const T& value) {
        if constexpr (std::is_same_v<T, std::uint8_t>) write_u8(value);
        else if constexpr (std::is_same_v<T, std::uint16_t>) write_u16(value);
        else if constexpr (std::is_same_v<T, std::uint32_t>) write_u32(value);
        else if constexpr (std::is_same_v<T, std::uint64_t>) write_u64(value);
        else if constexpr (std::is_same_v<T, float>) write_f32(value);
        else if constexpr (std::is_same_v<T, double>) write_f64(value);
        else if constexpr (std::is_same_v<T, bool>) write_bool(value);
        else if constexpr (std::is_enum_v<T>) write_u8(static_cast<std::uint8_t>(value));
        else {
            write_raw<T>(value);
        }
    }

private:
    template<typename T>
    void write_raw(const T& value) {
        static_assert(std::is_trivially_copyable_v<T>);
        const auto* ptr = reinterpret_cast<const std::uint8_t*>(&value);
        buffer.insert(buffer.end(), ptr, ptr + sizeof(T));
    }
};

struct BinaryReader {
    const std::vector<std::uint8_t>& buffer;
    std::size_t pos{0};
    bool error{false};
    std::uint16_t file_version{0};

    explicit BinaryReader(const std::vector<std::uint8_t>& buf) : buffer(buf) {}

    bool can_read(std::size_t n) const {
        if (error) return false;
        return n <= buffer.size() && pos <= buffer.size() - n;
    }

    std::uint8_t read_u8() { return read_raw<std::uint8_t>(); }
    std::uint16_t read_u16() { return read_raw<std::uint16_t>(); }
    std::uint32_t read_u32() { return read_raw<std::uint32_t>(); }
    std::uint64_t read_u64() { return read_raw<std::uint64_t>(); }
    float read_f32() { return read_raw<float>(); }
    double read_f64() { return read_raw<double>(); }
    bool read_bool() { return read_u8() != 0; }

    std::string read_string() {
        std::uint32_t size = read_u32();
        if (error || size > kMaxStringBytes || !can_read(size)) {
            error = true;
            return "";
        }
        std::string str(reinterpret_cast<const char*>(&buffer[pos]), size);
        pos += size;
        return str;
    }
    
    template<typename T, typename Deserializer>
    std::vector<T> read_vector(Deserializer deserializer) {
        std::uint32_t size = read_u32();
        if (error || size > kMaxVectorItems) {
            error = true;
            return {};
        }
        std::vector<T> vec;
        vec.reserve(size);
        for (std::uint32_t i = 0; i < size; ++i) {
            vec.push_back(deserializer(*this));
            if (error) break;
        }
        return vec;
    }

    template<typename T>
    std::vector<T> read_vector() {
        std::uint32_t size = read_u32();
        if (error || size > kMaxVectorItems) {
            error = true;
            return {};
        }
        std::vector<T> vec;
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::size_t bytes = static_cast<std::size_t>(size) * sizeof(T);
            if (!can_read(bytes)) {
                error = true;
                return {};
            }
            vec.resize(size);
            std::memcpy(vec.data(), &buffer[pos], bytes);
            pos += bytes;
        }
        return vec;
    }

    template<typename E>
    E read_enum_checked(bool (*validator)(std::uint8_t)) {
        auto raw = read_u8();
        if (!validator(raw)) {
            error = true;
            return static_cast<E>(0);
        }
        return static_cast<E>(raw);
    }

    template<typename T>
    T read() {
        if constexpr (std::is_same_v<T, std::uint8_t>) return read_u8();
        else if constexpr (std::is_same_v<T, std::uint16_t>) return read_u16();
        else if constexpr (std::is_same_v<T, std::uint32_t>) return read_u32();
        else if constexpr (std::is_same_v<T, std::uint64_t>) return read_u64();
        else if constexpr (std::is_same_v<T, float>) return read_f32();
        else if constexpr (std::is_same_v<T, double>) return read_f64();
        else if constexpr (std::is_same_v<T, bool>) return read_bool();
        else if constexpr (std::is_enum_v<T>) return static_cast<T>(read_u8());
        else {
            return read_raw<T>();
        }
    }

private:
    template<typename T>
    T read_raw() {
        if (!can_read(sizeof(T))) {
            error = true;
            return T{};
        }
        T value;
        std::memcpy(&value, &buffer[pos], sizeof(T));
        pos += sizeof(T);
        return value;
    }
};

} // namespace tachyon::runtime
