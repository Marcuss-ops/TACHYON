#include "tachyon/runtime/core/tbf_codec.h"
#include <fstream>
#include <cstring>

namespace tachyon::runtime {

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
};

struct BinaryReader {
    const std::vector<std::uint8_t>& buffer;
    std::size_t pos{0};
    bool error{false};

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
            writer.write_vector(layer.property_indices);
            writer.write(layer.flags);
        }
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
        writer.write_vector(expr.code);
        writer.write_vector(expr.constants);
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
    
    // 2. Metadata
    scene.project_id = reader.read_string();
    scene.project_name = reader.read_string();
    scene.scene_hash = reader.read<std::uint64_t>();
    
    // 3. Determinism
    scene.determinism = reader.read<DeterminismContract>();
    
    // 4. Graph
    auto edges = reader.read_vector<RenderGraph::Edge>();
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
            layer.property_indices = reader.read_vector<std::uint32_t>();
            layer.flags = reader.read<std::uint8_t>();
            comp.layers.push_back(std::move(layer));
        }
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
        CompiledExpression expr;
        expr.code = reader.read_vector<ExpressionInstr>();
        expr.constants = reader.read_vector<double>();
        scene.expressions.push_back(std::move(expr));
    }

    if (reader.error) return std::nullopt;

    scene.graph.compile();
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
