#include "tachyon/runtime/core/serialization/tbf_codec.h"
#include <fstream>
#include <cstring>

namespace tachyon::runtime {

namespace {

// Migration from version 1 to 2: Added mask_feather to CompiledLayer
CompiledLayer migrate_layer_v1_to_v2(const CompiledLayer& old) {
    CompiledLayer layer;
    layer.node = old.node;
    layer.type_id = old.type_id;
    layer.width = old.width;
    layer.height = old.height;
    layer.text_content = old.text_content;
    layer.font_id = old.font_id;
    layer.font_size = old.font_size;
    layer.fill_color = old.fill_color;
    layer.property_indices = old.property_indices;
    layer.flags = old.flags;
    layer.mask_feather = 0.0f; // Default value added in v2
    return layer;
}

// Migration from version 2 to 3: Added audio tracks to CompiledComposition
CompiledComposition migrate_composition_v2_to_v3(const CompiledComposition& old) {
    CompiledComposition comp;
    comp.node = old.node;
    comp.width = old.width;
    comp.height = old.height;
    comp.fps = old.fps;
    comp.duration = old.duration;
    comp.layers = old.layers;
    comp.camera_cuts = old.camera_cuts;
    // Audio tracks are initialized as empty in v3 for v2 scenes
    comp.audio_tracks = {}; 
    return comp;
}

} // namespace

CompiledScene TBFCodec::migrate(const CompiledScene& scene, std::uint16_t from_version) {
    CompiledScene migrated = scene;
    std::uint16_t ver = from_version;
    
    if (ver == 1) {
        // Migrate layers from v1 to v2
        for (auto& comp : migrated.compositions) {
            for (auto& layer : comp.layers) {
                // Re-create with default mask_feather
                layer.mask_feather = 0.0f;
            }
        }
        ver = 2;
    }
    
    if (ver == 2) {
        // Migrate compositions from v2 to v3
        for (auto& comp : migrated.compositions) {
            comp.audio_tracks = {}; // Explicitly initialize
        }
        ver = 3;
    }

    if (ver == 3) {
        // Migrate from v3 to v4: AudioTrackSpec gained playback_speed, pitch_shift, pitch_correct.
        // Default values already match the spec (1.0f, 1.0f, false), so this is a no-op structural migration.
        ver = 4;
    }

    if (ver == 4) {
        // Migrate from v4 to v5: Added text extrusion controls to CompiledLayer.
        // Existing scenes keep the zero defaults.
        ver = 5;
    }
    
    migrated.header.version = current_version();
    return migrated;
}

namespace {

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

    template<typename T>
    void write_vector(const std::vector<T>& vec) {
        write<std::uint32_t>(static_cast<std::uint32_t>(vec.size()));
        if constexpr (std::is_trivially_copyable_v<T>) {
            const auto* ptr = reinterpret_cast<const std::uint8_t*>(vec.data());
            buffer.insert(buffer.end(), ptr, ptr + (vec.size() * sizeof(T)));
        } else {
            for (const auto& item : vec) {
                // Manual serialization for non-POD vector elements would go here.
                // For simplicity in this DSL implementation, we'll assume most core structs 
                // in CompiledScene are either POD or have specific handlers.
            }
        }
    }
    
    // Specific handlers for complex types
    void write_node(const CompiledNode& node) {
        write(node.node_id);
        write(node.version);
        write(node.topo_index);
        write(node.dirty_mask);
        write(node.type);
        write_vector(node.dependencies);
        write_vector(node.dependents);
    }

    void write_track_binding(const spec::TrackBinding& b) {
        write_string(b.property_path);
        write_string(b.source_id);
        write_string(b.source_track_name);
        write(b.influence);
        write(b.enabled);
    }

    void write_time_remap(const spec::TimeRemapCurve& tr) {
        write(tr.enabled);
        write(tr.mode);
        write<std::uint32_t>(static_cast<std::uint32_t>(tr.keyframes.size()));
        for (const auto& kf : tr.keyframes) {
            write(kf.first);
            write(kf.second);
        }
    }

    void write_camera_cut(const spec::CameraCut& cut) {
        write_string(cut.camera_id);
        write(cut.start_seconds);
        write(cut.end_seconds);
    }

    void write_audio_effect(const spec::AudioEffectSpec& effect) {
        write_string(effect.type);
        write(effect.start_time.has_value());
        if (effect.start_time) write(*effect.start_time);
        write(effect.duration.has_value());
        if (effect.duration) write(*effect.duration);
        write(effect.gain_db.has_value());
        if (effect.gain_db) write(*effect.gain_db);
        write(effect.cutoff_freq_hz.has_value());
        if (effect.cutoff_freq_hz) write(*effect.cutoff_freq_hz);
    }

    void write_audio_track(const spec::AudioTrackSpec& track) {
        write_string(track.id);
        write_string(track.source_path);
        write(track.volume);
        write(track.pan);
        write(track.start_offset_seconds);
        
        write_vector(track.volume_keyframes);
        write_vector(track.pan_keyframes);
        
        write<std::uint32_t>(static_cast<std::uint32_t>(track.effects.size()));
        for (const auto& effect : track.effects) write_audio_effect(effect);

        // Version 4 fields
        write(track.playback_speed);
        write(track.pitch_shift);
        write(track.pitch_correct);
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
        } else {
            // Manual deserialization...
        }
        return vec;
    }

    CompiledNode read_node() {
        CompiledNode node;
        node.node_id = read<std::uint32_t>();
        node.version = read<std::uint32_t>();
        node.topo_index = read<std::uint32_t>();
        node.dirty_mask = read<std::uint64_t>();
        node.type = read<CompiledNodeType>();
        node.dependencies = read_vector<std::uint32_t>();
        node.dependents = read_vector<std::uint32_t>();
        return node;
    }

    spec::TrackBinding read_track_binding() {
        spec::TrackBinding b;
        b.property_path = read_string();
        b.source_id = read_string();
        b.source_track_name = read_string();
        b.influence = read<float>();
        b.enabled = read<bool>();
        return b;
    }

    spec::TimeRemapCurve read_time_remap() {
        spec::TimeRemapCurve tr;
        tr.enabled = read<bool>();
        tr.mode = read<spec::TimeRemapMode>();
        std::uint32_t kf_count = read<std::uint32_t>();
        for (std::uint32_t i = 0; i < kf_count; ++i) {
            float first = read<float>();
            float second = read<float>();
            tr.keyframes.push_back({first, second});
        }
        return tr;
    }

    spec::CameraCut read_camera_cut() {
        spec::CameraCut cut;
        cut.camera_id = read_string();
        cut.start_seconds = read<double>();
        cut.end_seconds = read<double>();
        return cut;
    }

    spec::AudioEffectSpec read_audio_effect() {
        spec::AudioEffectSpec effect;
        effect.type = read_string();
        if (read<bool>()) effect.start_time = read<double>();
        if (read<bool>()) effect.duration = read<double>();
        if (read<bool>()) effect.gain_db = read<float>();
        if (read<bool>()) effect.cutoff_freq_hz = read<float>();
        return effect;
    }

    spec::AudioTrackSpec read_audio_track() {
        spec::AudioTrackSpec track;
        track.id = read_string();
        track.source_path = read_string();
        track.volume = read<float>();
        track.pan = read<float>();
        track.start_offset_seconds = read<double>();
        
        track.volume_keyframes = read_vector<animation::Keyframe<float>>();
        track.pan_keyframes = read_vector<animation::Keyframe<float>>();
        
        std::uint32_t effect_count = read<std::uint32_t>();
        for (std::uint32_t i = 0; i < effect_count; ++i) track.effects.push_back(read_audio_effect());

        if (file_version >= 4) {
            track.playback_speed = read<float>();
            track.pitch_shift = read<float>();
            track.pitch_correct = read<bool>();
        }
        return track;
    }
};

} // namespace

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
    writer.write<std::uint32_t>(static_cast<std::uint32_t>(scene.compositions.size()));
    for (const auto& comp : scene.compositions) {
        writer.write_node(comp.node);
        writer.write(comp.width);
        writer.write(comp.height);
        writer.write(comp.fps);
        writer.write(comp.duration);
        
        writer.write<std::uint32_t>(static_cast<std::uint32_t>(comp.layers.size()));
        for (const auto& layer : comp.layers) {
            writer.write_node(layer.node);
            writer.write(layer.type_id);
            writer.write(layer.width);
            writer.write(layer.height);
            writer.write_string(layer.text_content);
            writer.write_string(layer.font_id);
            writer.write(layer.font_size);
            writer.write(layer.fill_color);
            writer.write(layer.extrusion_depth);
            writer.write(layer.bevel_size);
            writer.write(layer.hole_bevel_ratio);
            writer.write_vector(layer.property_indices);
            writer.write(layer.flags);

            // Unified Temporal & Tracking
            writer.write<std::uint32_t>(static_cast<std::uint32_t>(layer.track_bindings.size()));
            for (const auto& b : layer.track_bindings) writer.write_track_binding(b);
            writer.write_time_remap(layer.time_remap);
            writer.write(layer.frame_blend);
        }

        writer.write<std::uint32_t>(static_cast<std::uint32_t>(comp.camera_cuts.size()));
        for (const auto& cut : comp.camera_cuts) writer.write_camera_cut(cut);

        writer.write<std::uint32_t>(static_cast<std::uint32_t>(comp.audio_tracks.size()));
        for (const auto& track : comp.audio_tracks) writer.write_audio_track(track);
    }
    
    // 6. Property Tracks
    writer.write<std::uint32_t>(static_cast<std::uint32_t>(scene.property_tracks.size()));
    for (const auto& track : scene.property_tracks) {
        writer.write_node(track.node);
        writer.write_string(track.property_id);
        writer.write(track.kind);
        writer.write(track.constant_value);
        writer.write_vector(track.keyframes);
    }
    
    // 7. Expressions
    writer.write<std::uint32_t>(static_cast<std::uint32_t>(scene.expressions.size()));
    for (const auto& expr : scene.expressions) {
        writer.write_vector(expr.instructions);
        writer.write_vector(expr.constants);
        // Note: names are not serialized in this version
    }
    
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
    
    // 5. Compositions (Partial implementation for DSL demo)
    std::uint32_t comp_count = reader.read<std::uint32_t>();
    for (std::uint32_t i = 0; i < comp_count; ++i) {
        CompiledComposition comp;
        comp.node = reader.read_node();
        scene.graph.add_node(comp.node.node_id);
        comp.width = reader.read<std::uint32_t>();
        comp.height = reader.read<std::uint32_t>();
        comp.fps = reader.read<std::uint32_t>();
        comp.duration = reader.read<double>();
        
        std::uint32_t layer_count = reader.read<std::uint32_t>();
        for (std::uint32_t j = 0; j < layer_count; ++j) {
            CompiledLayer layer;
            layer.node = reader.read_node();
            scene.graph.add_node(layer.node.node_id);
            layer.type_id = reader.read<std::uint32_t>();
            layer.width = reader.read<std::uint32_t>();
            layer.height = reader.read<std::uint32_t>();
            layer.text_content = reader.read_string();
            layer.font_id = reader.read_string();
            layer.font_size = reader.read<float>();
            layer.fill_color = reader.read<ColorSpec>();
            if (reader.file_version >= 5) {
                layer.extrusion_depth = reader.read<float>();
                layer.bevel_size = reader.read<float>();
                layer.hole_bevel_ratio = reader.read<float>();
            } else {
                layer.extrusion_depth = 0.0f;
                layer.bevel_size = 0.0f;
                layer.hole_bevel_ratio = 0.0f;
            }
            layer.property_indices = reader.read_vector<std::uint32_t>();
            layer.flags = reader.read<std::uint8_t>();

            // Unified Temporal & Tracking
            std::uint32_t binding_count = reader.read<std::uint32_t>();
            for (std::uint32_t k = 0; k < binding_count; ++k) layer.track_bindings.push_back(reader.read_track_binding());
            layer.time_remap = reader.read_time_remap();
            layer.frame_blend = reader.read<spec::FrameBlendMode>();

            comp.layers.push_back(std::move(layer));
        }

        std::uint32_t cut_count = reader.read<std::uint32_t>();
        for (std::uint32_t k = 0; k < cut_count; ++k) comp.camera_cuts.push_back(reader.read_camera_cut());

        std::uint32_t audio_count = reader.read<std::uint32_t>();
        for (std::uint32_t k = 0; k < audio_count; ++k) comp.audio_tracks.push_back(reader.read_audio_track());

        scene.compositions.push_back(std::move(comp));
    }

    // 6. Property Tracks
    std::uint32_t track_count = reader.read<std::uint32_t>();
    for (std::uint32_t i = 0; i < track_count; ++i) {
        CompiledPropertyTrack track;
        track.node = reader.read_node();
        scene.graph.add_node(track.node.node_id);
        track.property_id = reader.read_string();
        track.kind = reader.read<CompiledPropertyTrack::Kind>();
        track.constant_value = reader.read<double>();
        track.keyframes = reader.read_vector<CompiledKeyframe>();
        scene.property_tracks.push_back(std::move(track));
    }

    // 7. Expressions
    std::uint32_t expression_count = reader.read<std::uint32_t>();
    for (std::uint32_t i = 0; i < expression_count; ++i) {
        expressions::Bytecode expr;
        expr.instructions = reader.read_vector<expressions::Instruction>();
        expr.constants = reader.read_vector<double>();
        // Note: names are not serialized in this version
        scene.expressions.push_back(std::move(expr));
    }

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
