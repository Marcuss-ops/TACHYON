#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace tachyon::runtime {

struct BinaryWriter {
    std::vector<std::uint8_t> buffer;

    template<typename T>
    void write(const T& value) {
        static_assert(std::is_trivially_copyable_v<T>);
        const auto* ptr = reinterpret_cast<const std::uint8_t*>(&value);
        buffer.insert(buffer.end(), ptr, ptr + sizeof(T));
    }

    void write_string(const std::string& str) {
        write<std::uint32_t>(static_cast<std::uint32_t>(str.size()));
        buffer.insert(buffer.end(), str.begin(), str.end());
    }

    template<typename T, typename Serializer>
    void write_vector(const std::vector<T>& vec, Serializer serializer) {
        write<std::uint32_t>(static_cast<std::uint32_t>(vec.size()));
        for (const auto& item : vec) {
            serializer(*this, item);
        }
    }

    template<typename T>
    void write_vector(const std::vector<T>& vec) {
        write<std::uint32_t>(static_cast<std::uint32_t>(vec.size()));
        if constexpr (std::is_trivially_copyable_v<T>) {
            const auto* ptr = reinterpret_cast<const std::uint8_t*>(vec.data());
            buffer.insert(buffer.end(), ptr, ptr + (vec.size() * sizeof(T)));
        } else {
            // Fallback for non-POD if we have a default serializer or something.
            // For now, let's keep it consistent with the existing logic.
        }
    }
};

struct BinaryReader {
    const std::vector<std::uint8_t>& buffer;
    std::size_t pos{0};
    bool error{false};
    std::uint16_t file_version{0};

    template<typename T>
    T read() {
        if (pos + sizeof(T) > buffer.size()) {
            error = true;
            return T{};
        }
        T value;
        std::memcpy(&value, &buffer[pos], sizeof(T));
        pos += sizeof(T);
        return value;
    }

    std::string read_string() {
        std::uint32_t size = read<std::uint32_t>();
        if (pos + size > buffer.size()) {
            error = true;
            return "";
        }
        std::string str(reinterpret_cast<const char*>(&buffer[pos]), size);
        pos += size;
        return str;
    }
    
    template<typename T, typename Deserializer>
    std::vector<T> read_vector(Deserializer deserializer) {
        std::uint32_t size = read<std::uint32_t>();
        if (error) return {};
        std::vector<T> vec;
        vec.reserve(size);
        for (std::uint32_t i = 0; i < size; ++i) {
            vec.push_back(deserializer(*this));
        }
        return vec;
    }

    template<typename T>
    std::vector<T> read_vector() {
        std::uint32_t size = read<std::uint32_t>();
        if (error) return {};
        std::vector<T> vec;
        if constexpr (std::is_trivially_copyable_v<T>) {
            if (pos + (size * sizeof(T)) > buffer.size()) {
                error = true;
                return {};
            }
            vec.resize(size);
            std::memcpy(vec.data(), &buffer[pos], size * sizeof(T));
            pos += size * sizeof(T);
        }
        return vec;
    }
};

} // namespace tachyon::runtime
