#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace tachyon::media {

struct MeshCompressionInput {
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> uvs;
    std::vector<std::uint32_t> indices;
};

struct MeshCompressionOutput {
    std::vector<std::uint8_t> bytes;
    std::string codec = "none";
};

class MeshCompressor {
public:
    virtual ~MeshCompressor() = default;
    virtual MeshCompressionOutput compress(const MeshCompressionInput& input) = 0;
};

MeshCompressor& none_mesh_compressor();

#if defined(TACHYON_ENABLE_DRACO)
MeshCompressor& draco_mesh_compressor();
#endif

} // namespace tachyon::media