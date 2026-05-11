#include "tachyon/media/compression/mesh_compressor.h"

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
            std::size_t num_verts = input.positions.size() / 3;
            int pos_att_id = mesh.AddAttribute(pos_attr, true, num_verts);
            auto* pos_ptr = mesh.attribute(pos_att_id);
            for (std::size_t i = 0; i < num_verts; ++i) {
                pos_ptr->SetAttributeValue(draco::AttributeValueIndex(static_cast<uint32_t>(i)),
                                           &input.positions[i * 3]);
            }
        }

        // Add normals
        if (!input.normals.empty()) {
            draco::PointAttribute norm_attr;
            norm_attr.Init(draco::GeometryAttribute::NORMAL, 3, draco::DT_FLOAT32, false,
                           sizeof(float) * 3);
            std::size_t num_verts = input.normals.size() / 3;
            int norm_att_id = mesh.AddAttribute(norm_attr, true, num_verts);
            auto* norm_ptr = mesh.attribute(norm_att_id);
            for (std::size_t i = 0; i < num_verts; ++i) {
                norm_ptr->SetAttributeValue(draco::AttributeValueIndex(static_cast<uint32_t>(i)),
                                            &input.normals[i * 3]);
            }
        }

        // Add texture coordinates (UVs)
        if (!input.uvs.empty()) {
            draco::PointAttribute uv_attr;
            uv_attr.Init(draco::GeometryAttribute::TEX_COORD, 2, draco::DT_FLOAT32, false,
                         sizeof(float) * 2);
            std::size_t num_verts = input.uvs.size() / 2;
            int uv_att_id = mesh.AddAttribute(uv_attr, true, num_verts);
            auto* uv_ptr = mesh.attribute(uv_att_id);
            for (std::size_t i = 0; i < num_verts; ++i) {
                uv_ptr->SetAttributeValue(draco::AttributeValueIndex(static_cast<uint32_t>(i)),
                                          &input.uvs[i * 2]);
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
                mesh.SetFace(draco::FaceIndex(static_cast<uint32_t>(i)), face);
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