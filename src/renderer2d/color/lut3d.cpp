#include "tachyon/renderer2d/color/lut3d.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>

namespace tachyon::renderer2d {

// ---------------------------------------------------------------------------
// Cube file parser
// ---------------------------------------------------------------------------

ParseResult<Lut3D> load_lut_cube(const std::filesystem::path& path) {
    ParseResult<Lut3D> result;

    std::ifstream file(path);
    if (!file.is_open()) {
        result.diagnostics.add_error(
            "lut3d.open_failed",
            "Failed to open .cube file: " + path.string());
        return result;
    }

    Lut3D lut;
    int declared_size = 0;
    std::string line;

    while (std::getline(file, line)) {
        // Strip Windows \r
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        // Skip blank lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Detect LUT_3D_SIZE keyword
        if (line.substr(0, 12) == "LUT_3D_SIZE ") {
            std::istringstream iss(line.substr(12));
            iss >> declared_size;
            if (declared_size < 2 || declared_size > 256) {
                result.diagnostics.add_error(
                    "lut3d.invalid_size",
                    "LUT_3D_SIZE out of range [2,256]: " + std::to_string(declared_size));
                return result;
            }
            lut.size = declared_size;
            lut.data.reserve(static_cast<std::size_t>(declared_size) *
                             static_cast<std::size_t>(declared_size) *
                             static_cast<std::size_t>(declared_size) * 3U);
            continue;
        }

        // Skip other keyword lines (TITLE, DOMAIN_MIN, DOMAIN_MAX, etc.)
        // A data line starts with a digit, '-', or '.'
        const char first = line[0];
        if (!((first >= '0' && first <= '9') || first == '-' || first == '.')) {
            continue;
        }

        // Parse triple float
        std::istringstream iss(line);
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        if (!(iss >> r >> g >> b)) {
            continue; // Malformed line — skip gracefully
        }

        lut.data.push_back(std::clamp(r, 0.0f, 1.0f));
        lut.data.push_back(std::clamp(g, 0.0f, 1.0f));
        lut.data.push_back(std::clamp(b, 0.0f, 1.0f));
    }

    if (lut.size < 2) {
        result.diagnostics.add_error(
            "lut3d.no_size",
            "LUT_3D_SIZE declaration not found in .cube file");
        return result;
    }

    const std::size_t expected = static_cast<std::size_t>(lut.size) *
                                 static_cast<std::size_t>(lut.size) *
                                 static_cast<std::size_t>(lut.size) * 3U;
    if (lut.data.size() != expected) {
        result.diagnostics.add_error(
            "lut3d.data_count_mismatch",
            "Expected " + std::to_string(expected / 3) + " entries, got " +
                std::to_string(lut.data.size() / 3));
        return result;
    }

    result.value = std::move(lut);
    return result;
}

// ---------------------------------------------------------------------------
// Trilinear sampling
// ---------------------------------------------------------------------------

Color apply_lut3d(const Lut3D& lut, Color input) {
    if (!lut.is_valid()) {
        return input;
    }

    const int N = lut.size;
    const float scale = static_cast<float>(N - 1);

    // Map 8-bit input to [0, N-1] float coordinates
    // CUBE format: rows are B-major (B varies fastest), then G, then R
    const float rb = (static_cast<float>(input.r) / 255.0f) * scale;
    const float gb = (static_cast<float>(input.g) / 255.0f) * scale;
    const float bb = (static_cast<float>(input.b) / 255.0f) * scale;

    const int r0 = std::clamp(static_cast<int>(rb), 0, N - 1);
    const int g0 = std::clamp(static_cast<int>(gb), 0, N - 1);
    const int b0 = std::clamp(static_cast<int>(bb), 0, N - 1);
    const int r1 = std::min(r0 + 1, N - 1);
    const int g1 = std::min(g0 + 1, N - 1);
    const int b1 = std::min(b0 + 1, N - 1);

    const float fr = rb - static_cast<float>(r0);
    const float fg = gb - static_cast<float>(g0);
    const float fb = bb - static_cast<float>(b0);

    // Helper: look up a triplet from the flat array (B-major order)
    // index = (r_idx * N * N + g_idx * N + b_idx) * 3
    const auto get = [&](int ri, int gi, int bi) -> std::array<float, 3> {
        const std::size_t idx = static_cast<std::size_t>(
            (ri * N * N + gi * N + bi) * 3);
        return {lut.data[idx], lut.data[idx + 1U], lut.data[idx + 2U]};
    };

    // 8 corners of the enclosing cell
    const auto c000 = get(r0, g0, b0);
    const auto c001 = get(r0, g0, b1);
    const auto c010 = get(r0, g1, b0);
    const auto c011 = get(r0, g1, b1);
    const auto c100 = get(r1, g0, b0);
    const auto c101 = get(r1, g0, b1);
    const auto c110 = get(r1, g1, b0);
    const auto c111 = get(r1, g1, b1);

    // Trilinear interpolation in b → g → r order
    const auto lerp3 = [](const std::array<float, 3>& a,
                           const std::array<float, 3>& b,
                           float t) -> std::array<float, 3> {
        return {a[0] + (b[0] - a[0]) * t,
                a[1] + (b[1] - a[1]) * t,
                a[2] + (b[2] - a[2]) * t};
    };

    // Lerp along B axis
    const auto c00 = lerp3(c000, c001, fb);
    const auto c01 = lerp3(c010, c011, fb);
    const auto c10 = lerp3(c100, c101, fb);
    const auto c11 = lerp3(c110, c111, fb);

    // Lerp along G axis
    const auto c0 = lerp3(c00, c01, fg);
    const auto c1 = lerp3(c10, c11, fg);

    // Lerp along R axis
    const auto c = lerp3(c0, c1, fr);

    return Color{
        static_cast<std::uint8_t>(std::clamp(std::lround(c[0] * 255.0f), 0L, 255L)),
        static_cast<std::uint8_t>(std::clamp(std::lround(c[1] * 255.0f), 0L, 255L)),
        static_cast<std::uint8_t>(std::clamp(std::lround(c[2] * 255.0f), 0L, 255L)),
        input.a // preserve alpha
    };
}

} // namespace tachyon::renderer2d
