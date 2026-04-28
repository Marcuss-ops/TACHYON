#include "tachyon/runtime/core/serialization/tbf_codec.h"
#include "src/runtime/core/serialization/binary_io.h"
#include "src/runtime/core/serialization/tbf_migration.h"
#include <fstream>
#include <cstring>

namespace tachyon::runtime {

CompiledNode read_node(BinaryReader& reader);
spec::TrackBinding read_track_binding(BinaryReader& reader);
spec::TimeRemapCurve read_time_remap(BinaryReader& reader);
spec::CameraCut read_camera_cut(BinaryReader& reader);
spec::AudioEffectSpec read_audio_effect(BinaryReader& reader);
spec::AudioTrackSpec read_audio_track(BinaryReader& reader);
void write_node(BinaryWriter& writer, const CompiledNode& node);
void write_track_binding(BinaryWriter& writer, const spec::TrackBinding& b);
void write_time_remap(BinaryWriter& writer, const spec::TimeRemapCurve& tr);
void write_camera_cut(BinaryWriter& writer, const spec::CameraCut& cut);
void write_audio_effect(BinaryWriter& writer, const spec::AudioEffectSpec& effect);
void write_audio_track(BinaryWriter& writer, const spec::AudioTrackSpec& track);

CompiledScene TBFCodec::migrate(const CompiledScene& scene, std::uint16_t from_version) {
    return TBFMigration::migrate(scene, from_version);
}

std::vector<std::uint8_t> TBFCodec::encode(const CompiledScene& scene) {
    BinaryWriter writer;
    
    // 1. Header
    writer.write(scene.header);
    
    // 2. Metadata
    writer.write_string(scene.project_id);
    writer.write_string(scene.project_name);
    writer.write(scene.scene_hash);
    
    // 3. Determinism
    writer.write(scene.determinism);
    
    // 4. Graph
    writer.write_vector(scene.graph.edges());
    writer.write_vector(scene.graph.topo_order());
    
    // 5. Compositions
    writer.write_vector(scene.compositions, [](BinaryWriter& w, const CompiledComposition& comp) {
        write_node(w, comp.node);
        w.write(comp.width);
        w.write(comp.height);
        w.write(comp.fps);
        w.write(comp.duration);
        
        w.write_vector(comp.layers, [](BinaryWriter& lw, const CompiledLayer& layer) {
            write_node(lw, layer.node);
            lw.write(layer.type_id);
            lw.write(layer.width);
            lw.write(layer.height);
            lw.write_string(layer.text_content);
            lw.write_string(layer.font_id);
            lw.write(layer.font_size);
            lw.write(layer.fill_color);
            lw.write(layer.extrusion_depth);
            lw.write(layer.bevel_size);
            lw.write(layer.hole_bevel_ratio);
            lw.write_vector(layer.property_indices);
            lw.write(layer.flags);

            lw.write_vector(layer.track_bindings, write_track_binding);
            write_time_remap(lw, layer.time_remap);
            lw.write(layer.frame_blend);
        });

        w.write_vector(comp.camera_cuts, write_camera_cut);
        w.write_vector(comp.audio_tracks, write_audio_track);
    });
    
    // 6. Property Tracks
    writer.write_vector(scene.property_tracks, [](BinaryWriter& w, const CompiledPropertyTrack& track) {
        write_node(w, track.node);
        w.write_string(track.property_id);
        w.write(track.kind);
        w.write(track.constant_value);
        w.write_vector(track.keyframes);
    });
    
    // 7. Expressions
    writer.write_vector(scene.expressions, [](BinaryWriter& w, const expressions::Bytecode& expr) {
        w.write_vector(expr.instructions);
        w.write_vector(expr.constants);
    });
    
    return std::move(writer.buffer);
}

std::optional<CompiledScene> TBFCodec::decode(const std::vector<std::uint8_t>& buffer) {
    BinaryReader reader{buffer};
    CompiledScene scene;
    
    // 1. Header & Validation
    scene.header = reader.read<CompiledSceneHeader>();
    if (scene.header.magic != 0x54414348U) return std::nullopt;
    if (scene.header.layout_checksum != CompiledScene::calculate_layout_checksum()) return std::nullopt;
    reader.file_version = scene.header.version;
    
    // 2. Metadata
    scene.project_id = reader.read_string();
    scene.project_name = reader.read_string();
    scene.scene_hash = reader.read<std::uint64_t>();
    
    // 3. Determinism
    scene.determinism = reader.read<DeterminismContract>();
    
    // 4. Graph
    auto edges = reader.read_vector<RuntimeRenderGraph::Edge>();
    auto topo = reader.read_vector<std::uint32_t>();
    for (const auto& edge : edges) {
        scene.graph.add_edge(edge.from, edge.to, edge.structural);
    }
    (void)topo;
    
    // 5. Compositions
    scene.compositions = reader.read_vector<CompiledComposition>([&scene](BinaryReader& r) {
        CompiledComposition comp;
        comp.node = read_node(r);
        scene.graph.add_node(comp.node.node_id);
        comp.width = r.read<std::uint32_t>();
        comp.height = r.read<std::uint32_t>();
        comp.fps = r.read<std::uint32_t>();
        comp.duration = r.read<double>();
        
        comp.layers = r.read_vector<CompiledLayer>([&scene](BinaryReader& lr) {
            CompiledLayer layer;
            layer.node = read_node(lr);
            scene.graph.add_node(layer.node.node_id);
            layer.type_id = lr.read<std::uint32_t>();
            layer.width = lr.read<std::uint32_t>();
            layer.height = lr.read<std::uint32_t>();
            layer.text_content = lr.read_string();
            layer.font_id = lr.read_string();
            layer.font_size = lr.read<float>();
            layer.fill_color = lr.read<ColorSpec>();
            if (lr.file_version >= 5) {
                layer.extrusion_depth = lr.read<float>();
                layer.bevel_size = lr.read<float>();
                layer.hole_bevel_ratio = lr.read<float>();
            } else {
                layer.extrusion_depth = 0.0f;
                layer.bevel_size = 0.0f;
                layer.hole_bevel_ratio = 0.0f;
            }
            layer.property_indices = lr.read_vector<std::uint32_t>();
            layer.flags = lr.read<std::uint8_t>();

            layer.track_bindings = lr.read_vector<spec::TrackBinding>(read_track_binding);
            layer.time_remap = read_time_remap(lr);
            layer.frame_blend = lr.read<spec::FrameBlendMode>();

            return layer;
        });

        comp.camera_cuts = r.read_vector<spec::CameraCut>(read_camera_cut);
        comp.audio_tracks = r.read_vector<spec::AudioTrackSpec>(read_audio_track);

        return comp;
    });

    // 6. Property Tracks
    scene.property_tracks = reader.read_vector<CompiledPropertyTrack>([&scene](BinaryReader& r) {
        CompiledPropertyTrack track;
        track.node = read_node(r);
        scene.graph.add_node(track.node.node_id);
        track.property_id = r.read_string();
        track.kind = r.read<CompiledPropertyTrack::Kind>();
        track.constant_value = r.read<double>();
        track.keyframes = r.read_vector<CompiledKeyframe>();
        return track;
    });

    // 7. Expressions
    scene.expressions = reader.read_vector<expressions::Bytecode>([](BinaryReader& r) {
        expressions::Bytecode expr;
        expr.instructions = r.read_vector<expressions::Instruction>();
        expr.constants = r.read_vector<double>();
        return expr;
    });

    if (reader.error) return std::nullopt;

    scene.graph.compile();

    if (scene.header.version < current_version()) {
        scene = migrate(scene, scene.header.version);
    }

    return scene;
}

bool TBFCodec::save_to_file(const CompiledScene& scene, const std::filesystem::path& path) {
    auto buffer = encode(scene);
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    return true;
}

std::optional<CompiledScene> TBFCodec::load_from_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return std::nullopt;
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) return std::nullopt;
    return decode(buffer);
}

} // namespace tachyon::runtime
