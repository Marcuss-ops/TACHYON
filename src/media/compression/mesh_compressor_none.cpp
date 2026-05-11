#include "tachyon/media/compression/mesh_compressor.h"

#include <cstring>
#include <type_traits>

namespace tachyon::media {
namespace {

class NoneMeshCompressor final : public MeshCompressor {
public:
    MeshCompressionOutput compress(const MeshCompressionInput& input) override {
        MeshCompressionOutput output;
        output.codec = "none";

        const auto append_bytes = [&output](const auto& values) {
            const auto* data = reinterpret_cast<const std::uint8_t*>(values.data());
            const std::size_t bytes = values.size() * sizeof(typename std::decay_t<decltype(values)>::value_type);
            output.bytes.insert(output.bytes.end(), data, data + bytes);
        };

        append_bytes(input.positions);
        append_bytes(input.normals);
        append_bytes(input.uvs);
        append_bytes(input.indices);

        return output;
    }
};

} // namespace

MeshCompressor& none_mesh_compressor() {
    static NoneMeshCompressor compressor;
    return compressor;
}

} // namespace tachyon::media