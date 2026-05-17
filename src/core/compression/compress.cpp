#include "tachyon/core/compression/compress.h"
#include <zstd.h>
#include <cstring>

namespace tachyon::compression {

static constexpr std::uint32_t kZstdMagic = 0xFD2FB528U;

std::vector<std::uint8_t> compress(std::span<const std::uint8_t> data, int level) {
    auto bound = ZSTD_compressBound(data.size());
    std::vector<std::uint8_t> out(bound);
    auto written = ZSTD_compress(out.data(), bound, data.data(), data.size(), level);
    if (ZSTD_isError(written)) return {};
    out.resize(written);
    return out;
}

std::optional<std::vector<std::uint8_t>> decompress(std::span<const std::uint8_t> compressed) {
    unsigned long long content_size = ZSTD_getFrameContentSize(compressed.data(), compressed.size());
    if (content_size == ZSTD_CONTENTSIZE_ERROR) return std::nullopt;

    std::size_t out_size = (content_size == ZSTD_CONTENTSIZE_UNKNOWN)
                           ? compressed.size() * 4
                           : static_cast<std::size_t>(content_size);

    std::vector<std::uint8_t> out(out_size);
    auto written = ZSTD_decompress(out.data(), out.size(), compressed.data(), compressed.size());
    if (ZSTD_isError(written)) return std::nullopt;
    out.resize(written);
    return out;
}

std::optional<std::vector<std::uint8_t>> try_decompress(std::span<const std::uint8_t> data) {
    if (data.size() < 4) return std::nullopt;
    std::uint32_t magic = 0;
    std::memcpy(&magic, data.data(), 4);
    if (magic != kZstdMagic) return std::nullopt;
    return decompress(data);
}

} // namespace tachyon::compression
