#include "tachyon/media/compression/mesh_compressor.h"

#ifdef TACHYON_ENABLE_DRACO
#include <draco/compression/encode.h>
#include <draco/core/cycle_timer.h>
#include <draco/mesh/mesh.h>

namespace tachyon::media {

class DracoMeshCompressor : public MeshCompressor {
public:
    MeshCompressionOutput compress(const MeshCompressionInput& input) override {
        MeshCompressionOutput output;
        output.codec = "draco";

        draco::Mesh mesh;
        
        // Add positions
        if (!input.positions.empty()) {
            draco::PointAttribute pos_attr;
            pos_attr.Init(draco::GeometryAttribute::POSITION, 3, draco::DT_FLOAT32, false,
                          sizeof(float) * 3);
            int pos_att_id = mesh.AddAttribute(pos_attr, true, input.positions.size() / 3);
            auto* pos_ptr = mesh.attribute(pos_att_id);
            for (std::size_t i = 0; i < input.positions.size() / 3; ++i) {
                pos_ptr->SetAttributeValue(draco::AttributeValueIndex(i), &input.positions[i * 3]);
            }
        }

        // Add normals
        if (!input.normals.empty()) {
            draco::PointAttribute norm_attr;
            norm_attr.Init(draco::GeometryAttribute::NORMAL, 3, draco::DT_FLOAT32, false,
                           sizeof(float) * 3);
            int norm_att_id = mesh.AddAttribute(norm_attr, true, input.normals.size() / 3);
            auto* norm_ptr = mesh.attribute(norm_att_id);
            for (std::size_t i = 0; i < input.normals.size() / 3; ++i) {
                norm_ptr->SetAttributeValue(draco::AttributeValueIndex(i), &input.normals[i * 3]);
            }
        }

        // Add indices (faces)
        if (!input.indices.empty()) {
            mesh.SetNumFaces(input.indices.size() / 3);
            for (std::size_t i = 0; i < input.indices.size() / 3; ++i) {
                draco::Mesh::Face face;
                face[0] = draco::PointIndex(input.indices[i * 3 + 0]);
                face[1] = draco::PointIndex(input.indices[i * 3 + 1]);
                face[2] = draco::PointIndex(input.indices[i * 3 + 2]);
                mesh.SetFace(draco::FaceIndex(i), face);
            }
        }

        draco::Encoder encoder;
        // Default compression speed/level
        encoder.SetSpeedOptions(5, 5);
        
        draco::EncoderBuffer buffer;
        if (encoder.EncodeMeshToBuffer(mesh, &buffer).ok()) {
            output.bytes.assign(buffer.data(), buffer.data() + buffer.size());
        }

        return output;
    }
};

MeshCompressor& draco_mesh_compressor() {
    static DracoMeshCompressor instance;
    return instance;
}

} // namespace tachyon::media

#else

namespace tachyon::media {
MeshCompressor& draco_mesh_compressor() {
    return none_mesh_compressor();
}
} // namespace tachyon::media

#endif

