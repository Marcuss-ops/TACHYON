#pragma once

#include "tachyon/renderer2d/framebuffer.h"
#include "tachyon/runtime/diagnostics.h"

#include <filesystem>
#include <vector>

namespace tachyon::renderer2d {

/// A 3D color lookup table loaded from a .cube file.
///
/// Data layout: flat array of size*size*size entries, each entry is 3 floats
/// (R, G, B) in [0,1]. Index order is B-major (blue axis varies fastest),
/// as specified by the Adobe/Resolve CUBE format:
///   entry_index = (r_idx * size * size + g_idx * size + b_idx)
///   data offset = entry_index * 3
struct Lut3D {
    int size{0};
    std::vector<float> data; // flat: [r, g, b] per grid point — B-major

    [[nodiscard]] bool is_valid() const noexcept {
        return size >= 2 &&
               static_cast<int>(data.size()) == size * size * size * 3;
    }
};

/// Load a 3D LUT from an Adobe/DaVinci .cube file.
/// Returns a ParseResult with the loaded Lut3D on success, or an error diagnostic.
[[nodiscard]] ParseResult<Lut3D> load_lut_cube(const std::filesystem::path& path);

/// Apply a 3D LUT to an 8-bit RGBA colour using trilinear interpolation.
/// Input alpha channel is preserved unchanged.
[[nodiscard]] Color apply_lut3d(const Lut3D& lut, Color input);

} // namespace tachyon::renderer2d
