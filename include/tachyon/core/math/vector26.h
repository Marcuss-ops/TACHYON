#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <ostream>
#include <string>

namespace tachyon {
namespace math {

class Vector26 {
public:
    using value_type = float;
    using container_type = std::array<value_type, 26>;
    using iterator = container_type::iterator;
    using const_iterator = container_type::const_iterator;

    Vector26() = default;

    Vector26(const container_type& values) : m_data(values) {}

    template<typename... Args>
    constexpr Vector26(Args... args) : m_data{static_cast<value_type>(args)...} {
        static_assert(sizeof...(Args) == 26, "Vector26 requires exactly 26 components");
    }

    static constexpr Vector26 zero() {
        Vector26 v;
        v.m_data.fill(0.0f);
        return v;
    }

    static constexpr Vector26 one() {
        Vector26 v;
        v.m_data.fill(1.0f);
        return v;
    }

    value_type& operator[](std::size_t index) { return m_data[index]; }
    const value_type& operator[](std::size_t index) const { return m_data[index]; }

    value_type& at(std::size_t index) { return m_data.at(index); }
    const value_type& at(std::size_t index) const { return m_data.at(index); }

    value_type& operator[](char letter) {
        return m_data[static_cast<std::size_t>(std::tolower(letter) - 'a')];
    }
    const value_type& operator[](char letter) const {
        return m_data[static_cast<std::size_t>(std::tolower(letter) - 'a')];
    }

    iterator begin() { return m_data.begin(); }
    iterator end() { return m_data.end(); }
    const_iterator begin() const { return m_data.begin(); }
    const_iterator end() const { return m_data.end(); }
    const_iterator cbegin() const { return m_data.cbegin(); }
    const_iterator cend() const { return m_data.cend(); }

    constexpr Vector26 operator-() const {
        Vector26 result;
        for (std::size_t i = 0; i < 26; ++i) {
            result.m_data[i] = -m_data[i];
        }
        return result;
    }

    constexpr Vector26 operator+(const Vector26& other) const {
        Vector26 result;
        for (std::size_t i = 0; i < 26; ++i) {
            result.m_data[i] = m_data[i] + other.m_data[i];
        }
        return result;
    }

    constexpr Vector26 operator-(const Vector26& other) const {
        Vector26 result;
        for (std::size_t i = 0; i < 26; ++i) {
            result.m_data[i] = m_data[i] - other.m_data[i];
        }
        return result;
    }

    constexpr Vector26 operator*(value_type scalar) const {
        Vector26 result;
        for (std::size_t i = 0; i < 26; ++i) {
            result.m_data[i] = m_data[i] * scalar;
        }
        return result;
    }

    constexpr Vector26 operator*(const Vector26& other) const {
        Vector26 result;
        for (std::size_t i = 0; i < 26; ++i) {
            result.m_data[i] = m_data[i] * other.m_data[i];
        }
        return result;
    }

    constexpr Vector26 operator/(value_type scalar) const {
        Vector26 result;
        for (std::size_t i = 0; i < 26; ++i) {
            result.m_data[i] = m_data[i] / scalar;
        }
        return result;
    }

    Vector26& operator+=(const Vector26& other) {
        for (std::size_t i = 0; i < 26; ++i) {
            m_data[i] += other.m_data[i];
        }
        return *this;
    }

    Vector26& operator-=(const Vector26& other) {
        for (std::size_t i = 0; i < 26; ++i) {
            m_data[i] -= other.m_data[i];
        }
        return *this;
    }

    Vector26& operator*=(value_type scalar) {
        for (std::size_t i = 0; i < 26; ++i) {
            m_data[i] *= scalar;
        }
        return *this;
    }

    Vector26& operator/=(value_type scalar) {
        for (std::size_t i = 0; i < 26; ++i) {
            m_data[i] /= scalar;
        }
        return *this;
    }

    [[nodiscard]] value_type length_squared() const {
        value_type sum = 0.0f;
        for (const auto& val : m_data) {
            sum += val * val;
        }
        return sum;
    }

    [[nodiscard]] value_type length() const {
        return std::sqrt(length_squared());
    }

    [[nodiscard]] Vector26 normalized() const {
        value_type len = length();
        if (len > 0.0f) {
            return *this / len;
        }
        return zero();
    }

    [[nodiscard]] static value_type dot(const Vector26& a, const Vector26& b) {
        value_type sum = 0.0f;
        for (std::size_t i = 0; i < 26; ++i) {
            sum += a.m_data[i] * b.m_data[i];
        }
        return sum;
    }

    [[nodiscard]] std::string to_string() const {
        std::string result = "(";
        for (std::size_t i = 0; i < 26; ++i) {
            result += std::to_string(m_data[i]);
            if (i < 25) result += ", ";
        }
        result += ")";
        return result;
    }

private:
    container_type m_data{};
};

inline Vector26 operator*(typename Vector26::value_type scalar, const Vector26& v) {
    return v * scalar;
}

inline std::ostream& operator<<(std::ostream& out, const Vector26& v) {
    out << "(";
    for (std::size_t i = 0; i < 26; ++i) {
        out << v[i];
        if (i < 25) out << ", ";
    }
    out << ")";
    return out;
}

} // namespace math
} // namespace tachyon
