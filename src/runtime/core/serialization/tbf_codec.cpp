#include "tachyon/runtime/core/serialization/tbf_codec.h"
#include "binary_io.h"
#include <fstream>
#include <cstring>
#include <algorithm>
#include <iostream>

namespace tachyon::runtime {

namespace {

// Enum validation helpers
template<typename E>
bool is_valid_enum(std::uint64_t value) {
    if constexpr (std::is_same_v<E, CompiledNodeType>) {
        return value <= static_cast<std::uint64_t>(CompiledNodeType::Unknown);
    } else if constexpr (std::is_same_v<E, CompiledPropertyTrack::Kind>) {
        return value <= static_cast<std::uint64_t>(CompiledPropertyTrack::Kind::Binding);
    } else if constexpr (std::is_same_v<E, spec::FrameBlendMode>) {
        return value <= static_cast<std::uint64_t>(spec::FrameBlendMode::OpticalFlow);
    } else if constexpr (std::is_same_v<E, spec::TimeRemapMode>) {
        return value <= static_cast<std::uint64_t>(spec::TimeRemapMode::OpticalFlow);
    } else if constexpr (std::is_same_v<E, DeterminismContract::FloatingPointMode>) {
        return value <= static_cast<std::uint64_t>(DeterminismContract::FloatingPointMode::Relaxed);
    } else if constexpr (std::is_same_v<E, DeterminismContract::FmaPolicy>) {
        return value <= static_cast<std::uint64_t>(DeterminismContract::FmaPolicy::Enable);
    } else if constexpr (std::is_same_v<E, DeterminismContract::DenormalPolicy>) {
        return value <= static_cast<std::uint64_t>(DeterminismContract::DenormalPolicy::Preserve);
    } else if constexpr (std::is_same_v<E, DeterminismContract::RoundingMode>) {
        return value <= static_cast<std::uint64_t>(DeterminismContract::RoundingMode::Downward);
    } else if constexpr (std::is_same_v<E, DeterminismContract::PrngKind>) {
        return value <= static_cast<std::uint64_t>(DeterminismContract::PrngKind::PCG32);
    }
    return true;
}

// Robust Serialization Helpers
struct TBFReader : public BinaryReader {
    using BinaryReader::BinaryReader;

    template<typename E>
    E read_enum() {
        return read_enum_checked<E>(is_valid_enum<E>);
    }

    CompiledNode read_node() {
        CompiledNode node;
        node.node_id = read_u32();
        node.version = read_u32();
        node.topo_index = read_u32();
        node.dirty_mask = read_u64();
        node.type = read_enum<CompiledNodeType>();
        node.dependencies = read_vector<std::uint32_t>();
        node.dependents = read_vector<std::uint32_t>();
        return node;
    }

    spec::TrackBinding read_track_binding() {
        spec::TrackBinding b;
        b.property_path = read_string();
        b.source_id = read_string();
        b.source_track_name = read_string();
        b.influence = read_f32();
        b.enabled = read_bool();
        return b;
    }

    spec::TimeRemapCurve read_time_remap() {
        spec::TimeRemapCurve tr;
        tr.enabled = read_bool();
        tr.mode = read_enum<spec::TimeRemapMode>();
        std::uint32_t kf_count = read_u32();
        if (kf_count > kMaxVectorItems) { error = true; return tr; }
        for (std::uint32_t i = 0; i < kf_count; ++i) {
            float first = read_f32();
            float second = read_f32();
            tr.keyframes.push_back({first, second});
        }
        return tr;
    }


    spec::AudioEffectSpec read_audio_effect() {
        spec::AudioEffectSpec effect;
        effect.type = read_string();
        if (read_bool()) effect.start_time = read_f64();
        if (read_bool()) effect.duration = read_f64();
        if (read_bool()) effect.gain_db = read_f32();
        if (read_bool()) effect.cutoff_freq_hz = read_f32();
        return effect;
    }

    spec::AudioTrackSpec read_audio_track() {
        spec::AudioTrackSpec track;
        track.id = read_string();
        track.source_path = read_string();
        track.volume = read_f32();
        track.pan = read_f32();
        track.start_offset_seconds = read_f64();
        
        track.volume_keyframes = read_vector<animation::Keyframe<float>>([](BinaryReader& r) {
             animation::Keyframe<float> kf;
             kf.time = r.read_f64();
             kf.value = r.read_f32();
             return kf;
        });
        track.pan_keyframes = read_vector<animation::Keyframe<float>>([](BinaryReader& r) {
             animation::Keyframe<float> kf;
             kf.time = r.read_f64();
             kf.value = r.read_f32();
             return kf;
        });
        
        std::uint32_t effect_count = read_u32();
        if (effect_count > kMaxVectorItems) { error = true; return track; }
        for (std::uint32_t i = 0; i < effect_count; ++i) track.effects.push_back(read_audio_effect());

        if (file_version >= 4) {
            track.playback_speed = read_f32();
            track.pitch_shift = read_f32();
            track.pitch_correct = read_bool();
        }
        return track;
    }

    AnimatedScalarSpec read_animated_scalar() {
        AnimatedScalarSpec spec;
        if (read_bool()) spec.value = read_f64();
        std::uint32_t kf_count = read_u32();
        for (std::uint32_t i = 0; i < kf_count; ++i) {
            ScalarKeyframeSpec kf;
            kf.time = read_f64();
            kf.value = read_f64();
            kf.easing = read_enum<animation::EasingPreset>();
            spec.keyframes.push_back(kf);
        }
        if (read_bool()) spec.expression = read_string();
        return spec;
    }

    AnimatedColorSpec read_animated_color() {
        AnimatedColorSpec spec;
        if (read_bool()) {
            ColorSpec col;
            col.r = read_u8(); col.g = read_u8(); col.b = read_u8(); col.a = read_u8();
            spec.value = col;
        }
        std::uint32_t kf_count = read_u32();
        for (std::uint32_t i = 0; i < kf_count; ++i) {
            ColorKeyframeSpec kf;
            kf.time = read_f64();
            kf.value.r = read_u8(); kf.value.g = read_u8(); kf.value.b = read_u8(); kf.value.a = read_u8();
            kf.easing = read_enum<animation::EasingPreset>();
            spec.keyframes.push_back(kf);
        }
        if (read_bool()) spec.expression = read_string();
        return spec;
    }
};

struct TBFWriter : public BinaryWriter {
    using BinaryWriter::BinaryWriter;

    void write_node(const CompiledNode& node) {
        write_u32(node.node_id);
        write_u32(node.version);
        write_u32(node.topo_index);
        write_u64(node.dirty_mask);
        write_u8(static_cast<std::uint8_t>(node.type));
        write_vector(node.dependencies);
        write_vector(node.dependents);
    }

    void write_track_binding(const spec::TrackBinding& b) {
        write_string(b.property_path);
        write_string(b.source_id);
        write_string(b.source_track_name);
        write_f32(b.influence);
        write_bool(b.enabled);
    }

    void write_time_remap(const spec::TimeRemapCurve& tr) {
        write_bool(tr.enabled);
        write_u8(static_cast<std::uint8_t>(tr.mode));
        write_u32(static_cast<std::uint32_t>(std::min<std::size_t>(tr.keyframes.size(), kMaxVectorItems)));
        for (const auto& kf : tr.keyframes) {
            write_f32(kf.first);
            write_f32(kf.second);
        }
    }


    void write_audio_effect(const spec::AudioEffectSpec& effect) {
        write_string(effect.type);
        write_bool(effect.start_time.has_value());
        if (effect.start_time) write_f64(*effect.start_time);
        write_bool(effect.duration.has_value());
        if (effect.duration) write_f64(*effect.duration);
        write_bool(effect.gain_db.has_value());
        if (effect.gain_db) write_f32(*effect.gain_db);
        write_bool(effect.cutoff_freq_hz.has_value());
        if (effect.cutoff_freq_hz) write_f32(*effect.cutoff_freq_hz);
    }

    void write_audio_track(const spec::AudioTrackSpec& track) {
        write_string(track.id);
        write_string(track.source_path);
        write_f32(track.volume);
        write_f32(track.pan);
        write_f64(track.start_offset_seconds);
        
        write_vector(track.volume_keyframes, [](BinaryWriter& w, const animation::Keyframe<float>& kf) {
            w.write_f64(kf.time);
            w.write_f32(kf.value);
        });
        write_vector(track.pan_keyframes, [](BinaryWriter& w, const animation::Keyframe<float>& kf) {
            w.write_f64(kf.time);
            w.write_f32(kf.value);
        });
        
        write_u32(static_cast<std::uint32_t>(std::min<std::size_t>(track.effects.size(), kMaxVectorItems)));
        for (const auto& effect : track.effects) write_audio_effect(effect);

        write_f32(track.playback_speed);
        write_f32(track.pitch_shift);
        write_bool(track.pitch_correct);
    }

    void write_animated_scalar(const AnimatedScalarSpec& spec) {
        write_bool(spec.value.has_value());
        if (spec.value) write_f64(*spec.value);
        write_u32(static_cast<std::uint32_t>(spec.keyframes.size()));
        for (const auto& kf : spec.keyframes) {
            write_f64(kf.time);
            write_f64(kf.value);
            write_u8(static_cast<std::uint8_t>(kf.easing));
        }
        write_bool(spec.expression.has_value());
        if (spec.expression) write_string(*spec.expression);
    }

    void write_animated_color(const AnimatedColorSpec& spec) {
        write_bool(spec.value.has_value());
        if (spec.value) {
            write_u8(spec.value->r); write_u8(spec.value->g); write_u8(spec.value->b); write_u8(spec.value->a);
        }
        write_u32(static_cast<std::uint32_t>(spec.keyframes.size()));
        for (const auto& kf : spec.keyframes) {
            write_f64(kf.time);
            write_u8(kf.value.r); write_u8(kf.value.g); write_u8(kf.value.b); write_u8(kf.value.a);
            write_u8(static_cast<std::uint8_t>(kf.easing));
        }
        write_bool(spec.expression.has_value());
        if (spec.expression) write_string(*spec.expression);
    }
};

} // namespace

CompiledScene TBFCodec::migrate(const CompiledScene& scene, std::uint16_t from_version) {
    CompiledScene migrated = scene;
    std::uint16_t ver = from_version;
    
    if (ver == 1) {
        for (auto& comp : migrated.compositions) {
            for (auto& layer : comp.layers) {
                layer.masks.feather = 0.0f;
            }
        }
        ver = 2;
    }
    
    if (ver == 2) {
        for (auto& comp : migrated.compositions) {
            comp.audio_tracks = {}; 
        }
        ver = 3;
    }

    if (ver == 3) {
        ver = 4;
    }
    
    migrated.header.version = current_version();
    return migrated;
}

std::vector<std::uint8_t> TBFCodec::encode(const CompiledScene& scene) {
    TBFWriter writer;
    
    // 1. Header (Partial write to avoid fragile struct copy)
    writer.write_u32(scene.header.magic);
    writer.write_u16(scene.header.version);
    writer.write_u16(scene.header.flags);
    writer.write_u32(scene.header.layout_version);
    writer.write_u32(scene.header.compiler_version);
    writer.write_u64(scene.header.layout_checksum);
    
    // 2. Metadata
    writer.write_string(scene.project_id);
    writer.write_string(scene.project_name);
    writer.write_u64(scene.scene_hash);
    
    // 3. Determinism
    writer.write_u16(static_cast<std::uint16_t>(scene.determinism.fp_mode));
    writer.write_u16(static_cast<std::uint16_t>(scene.determinism.fma_policy));
    writer.write_u16(static_cast<std::uint16_t>(scene.determinism.denormal_policy));
    writer.write_u16(static_cast<std::uint16_t>(scene.determinism.rounding_mode));
    writer.write_u16(static_cast<std::uint16_t>(scene.determinism.prng));
    writer.write_u64(scene.determinism.seed);
    writer.write_u32(scene.determinism.layout_version);
    writer.write_u32(scene.determinism.compiler_version);
    
    // 4. Graph
    writer.write_vector(scene.graph.edges(), [](BinaryWriter& w, const RuntimeRenderGraph::Edge& e) {
        w.write_u32(e.from);
        w.write_u32(e.to);
        w.write_bool(e.structural);
    });
    writer.write_vector(scene.graph.topo_order());
    
    // 5. Compositions
    writer.write_u32(static_cast<std::uint32_t>(std::min<std::size_t>(scene.compositions.size(), kMaxCompositions)));
    for (const auto& comp : scene.compositions) {
        writer.write_node(comp.node);
        writer.write_string(comp.composition_id);
        writer.write_string(comp.name);
        writer.write_u32(comp.width);
        writer.write_u32(comp.height);
        writer.write_u32(comp.fps);
        writer.write_f64(comp.duration);
        
        writer.write_u32(static_cast<std::uint32_t>(std::min<std::size_t>(comp.layers.size(), kMaxLayers)));
        for (const auto& layer : comp.layers) {
            writer.write_node(layer.node);
            writer.write_string(layer.name);
            writer.write_string(layer.asset_id);
            writer.write_u32(layer.type_id);
            writer.write_u32(layer.width);
            writer.write_u32(layer.height);
            writer.write_string(layer.text.content);
            writer.write_string(layer.text.font_id);
            writer.write_f32(layer.text.font_size);
            // ColorSpec
            writer.write_u8(layer.text.fill_color.r); writer.write_u8(layer.text.fill_color.g); writer.write_u8(layer.text.fill_color.b); writer.write_u8(layer.text.fill_color.a);
            writer.write_u8(layer.text.stroke_color.r); writer.write_u8(layer.text.stroke_color.g); writer.write_u8(layer.text.stroke_color.b); writer.write_u8(layer.text.stroke_color.a);
            writer.write_f32(layer.text.stroke_width);
            writer.write_vector(layer.property_indices);
            writer.write_u8(layer.flags);
            writer.write_f32(layer.masks.feather);
            writer.write_string(layer.blend_mode);
            writer.write_f64(layer.in_time);
            writer.write_f64(layer.out_time);
            writer.write_f64(layer.start_time);

            // Version 5 additions
            writer.write_bool(layer.parent_index.has_value());
            if (layer.parent_index) writer.write_u32(*layer.parent_index);
            writer.write_bool(layer.precomp_index.has_value());
            if (layer.precomp_index) writer.write_u32(*layer.precomp_index);

            // Version 5: Unified Effects
            writer.write_u32(static_cast<std::uint32_t>(std::min<std::size_t>(layer.effects.size(), kMaxVectorItems)));
            for (const auto& effect : layer.effects) {
                writer.write_bool(effect.enabled);
                writer.write_string(effect.type);
                
                std::vector<std::pair<std::string, AnimatedScalarSpec>> temp_scalars;
                std::vector<std::pair<std::string, AnimatedColorSpec>> temp_colors;
                std::vector<std::pair<std::string, std::string>> temp_strings;
                std::vector<std::pair<std::string, bool>> temp_bools;
                
                for (const auto& [k, v] : effect.params) {
                    if (std::holds_alternative<AnimatedScalarSpec>(v)) temp_scalars.push_back({k, std::get<AnimatedScalarSpec>(v)});
                    else if (std::holds_alternative<double>(v)) { AnimatedScalarSpec s; s.value = std::get<double>(v); temp_scalars.push_back({k, s}); }
                    else if (std::holds_alternative<AnimatedColorSpec>(v)) temp_colors.push_back({k, std::get<AnimatedColorSpec>(v)});
                    else if (std::holds_alternative<ColorSpec>(v)) { AnimatedColorSpec c; c.value = std::get<ColorSpec>(v); temp_colors.push_back({k, c}); }
                    else if (std::holds_alternative<std::string>(v)) temp_strings.push_back({k, std::get<std::string>(v)});
                    else if (std::holds_alternative<bool>(v)) temp_bools.push_back({k, std::get<bool>(v)});
                }

                writer.write_u32(static_cast<std::uint32_t>(temp_scalars.size()));
                for (const auto& [k, v] : temp_scalars) {
                    writer.write_string(k);
                    writer.write_animated_scalar(v);
                }
                
                writer.write_u32(static_cast<std::uint32_t>(temp_colors.size()));
                for (const auto& [k, v] : temp_colors) {
                    writer.write_string(k);
                    writer.write_animated_color(v);
                }
                
                writer.write_u32(static_cast<std::uint32_t>(temp_strings.size()));
                for (const auto& [k, v] : temp_strings) {
                    writer.write_string(k);
                    writer.write_string(v);
                }
                
                writer.write_u32(static_cast<std::uint32_t>(temp_bools.size()));
                for (const auto& [k, v] : temp_bools) {
                    writer.write_string(k);
                    writer.write_bool(v);
                }
            }

            // Unified Temporal & Tracking
            writer.write_vector(layer.temporal.track_bindings, [](BinaryWriter& w, const spec::TrackBinding& b) {
                TBFWriter& tw = static_cast<TBFWriter&>(w);
                tw.write_track_binding(b);
            });
            writer.write_time_remap(layer.temporal.time_remap);
            writer.write_u8(static_cast<std::uint8_t>(layer.temporal.frame_blend));
        }


        writer.write_vector(comp.audio_tracks, [](BinaryWriter& w, const spec::AudioTrackSpec& t) {
            TBFWriter& tw = static_cast<TBFWriter&>(w);
            tw.write_audio_track(t);
        });
    }
    
    // 6. Property Tracks
    writer.write_vector(scene.property_tracks, [](BinaryWriter& w, const CompiledPropertyTrack& t) {
        TBFWriter& tw = static_cast<TBFWriter&>(w);
        tw.write_node(t.node);
        tw.write_string(t.property_id);
        tw.write_u8(static_cast<std::uint8_t>(t.kind));
        tw.write_f64(t.constant_value);
        tw.write_vector(t.keyframes, [](BinaryWriter& w2, const CompiledKeyframe& kf) {
            w2.write_f64(kf.time); w2.write_f64(kf.value); w2.write_u32(kf.easing);
            w2.write_f64(kf.cx1); w2.write_f64(kf.cy1); w2.write_f64(kf.cx2); w2.write_f64(kf.cy2);
            w2.write_f64(kf.spring_stiffness); w2.write_f64(kf.spring_damping); w2.write_f64(kf.spring_mass);
        });
    });
    
    // 7. Expressions
    writer.write_vector(scene.expressions, [](BinaryWriter& w, const expressions::Bytecode& e) {
        w.write_vector(e.instructions, [](BinaryWriter& w2, const expressions::Instruction& instr) {
            w2.write_u8(static_cast<std::uint8_t>(instr.op));
            w2.write_u32(instr.data);
        });
        w.write_vector(e.constants);
        w.write_vector(e.names, [](BinaryWriter& w3, const std::string& name) {
            w3.write_string(name);
        });
    });

    // 8. Assets
    writer.write_vector(scene.assets, [](BinaryWriter& w, const AssetSpec& a) {
        w.write_string(a.id);
        w.write_string(a.path);
        w.write_string(a.type);
    });
    
    return std::move(writer.buffer);
}

std::optional<CompiledScene> TBFCodec::decode(const std::vector<std::uint8_t>& buffer) {
    TBFReader reader{buffer};
    CompiledScene scene;
    
    // 1. Header & Validation
    scene.header.magic = reader.read_u32();
    scene.header.version = reader.read_u16();
    scene.header.flags = reader.read_u16();
    scene.header.layout_version = reader.read_u32();
    scene.header.compiler_version = reader.read_u32();
    scene.header.layout_checksum = reader.read_u64();
    
    if (scene.header.magic != 0x54414348U) return std::nullopt;
    if (scene.header.layout_checksum != CompiledScene::calculate_layout_checksum()) return std::nullopt;
    reader.file_version = scene.header.version;
    
    // 2. Metadata
    scene.project_id = reader.read_string();
    scene.project_name = reader.read_string();
    scene.scene_hash = reader.read_u64();
    
    if (reader.error) return std::nullopt;
    
    // 3. Determinism
    scene.determinism.fp_mode = reader.read_enum<DeterminismContract::FloatingPointMode>();
    scene.determinism.fma_policy = reader.read_enum<DeterminismContract::FmaPolicy>();
    scene.determinism.denormal_policy = reader.read_enum<DeterminismContract::DenormalPolicy>();
    scene.determinism.rounding_mode = reader.read_enum<DeterminismContract::RoundingMode>();
    scene.determinism.prng = reader.read_enum<DeterminismContract::PrngKind>();
    scene.determinism.seed = reader.read_u64();
    scene.determinism.layout_version = reader.read_u32();
    scene.determinism.compiler_version = reader.read_u32();
    
    if (reader.error) { std::cerr << "TBF: Error reading determinism\n"; return std::nullopt; }

    // 4. Graph
    scene.graph.edges() = reader.read_vector<RuntimeRenderGraph::Edge>([](BinaryReader& r) {
        RuntimeRenderGraph::Edge e;
        e.from = r.read_u32();
        e.to = r.read_u32();
        e.structural = r.read_bool();
        return e;
    });
    scene.graph.topo_order() = reader.read_vector<std::uint32_t>();
    
    if (reader.error) { std::cerr << "TBF: Error reading graph\n"; return std::nullopt; }

    // 5. Compositions
    std::uint32_t comp_count = reader.read_u32();
    if (comp_count > kMaxCompositions) return std::nullopt;
    for (std::uint32_t i = 0; i < comp_count; ++i) {
        CompiledComposition comp;
        comp.node = reader.read_node();
        scene.graph.add_node(comp.node.node_id);
        comp.composition_id = reader.read_string();
        comp.name = reader.read_string();
        comp.width = reader.read_u32();
        comp.height = reader.read_u32();
        comp.fps = reader.read_u32();
        comp.duration = reader.read_f64();
        
        std::uint32_t layer_count = reader.read_u32();
        if (layer_count > kMaxLayers) return std::nullopt;
        for (std::uint32_t j = 0; j < layer_count; ++j) {
            CompiledLayer layer;
            layer.node = reader.read_node();
            scene.graph.add_node(layer.node.node_id);
            layer.name = reader.read_string();
            layer.asset_id = reader.read_string();
            layer.type_id = reader.read_u32();
            layer.width = reader.read_u32();
            layer.height = reader.read_u32();
            layer.text.content = reader.read_string();
            layer.text.font_id = reader.read_string();
            layer.text.font_size = reader.read_f32();
            // ColorSpec
            layer.text.fill_color.r = reader.read_u8(); layer.text.fill_color.g = reader.read_u8(); layer.text.fill_color.b = reader.read_u8(); layer.text.fill_color.a = reader.read_u8();
            layer.text.stroke_color.r = reader.read_u8(); layer.text.stroke_color.g = reader.read_u8(); layer.text.stroke_color.b = reader.read_u8(); layer.text.stroke_color.a = reader.read_u8();
            layer.text.stroke_width = reader.read_f32();
            layer.property_indices = reader.read_vector<std::uint32_t>();
            layer.flags = reader.read_u8();
            layer.masks.feather = reader.read_f32();
            layer.blend_mode = reader.read_string();
            layer.in_time = reader.read_f64();
            layer.out_time = reader.read_f64();
            layer.start_time = reader.read_f64();

            if (reader.file_version >= 5) {
                if (reader.read_bool()) layer.parent_index = reader.read_u32();
                if (reader.read_bool()) layer.precomp_index = reader.read_u32();

                std::uint32_t effect_count = reader.read_u32();
                if (effect_count > kMaxVectorItems) { reader.error = true; return std::nullopt; }
                for (std::uint32_t k = 0; k < effect_count; ++k) {
                    EffectSpec eff;
                    eff.enabled = reader.read_bool();
                    eff.type = reader.read_string();
                    
                    std::uint32_t scalar_count = reader.read_u32();
                    for (std::uint32_t s = 0; s < scalar_count; ++s) {
                        std::string key = reader.read_string();
                        eff.params[key] = reader.read_animated_scalar();
                    }
                    
                    std::uint32_t color_count = reader.read_u32();
                    for (std::uint32_t c = 0; c < color_count; ++c) {
                        std::string key = reader.read_string();
                        eff.params[key] = reader.read_animated_color();
                    }
                    
                    std::uint32_t string_count = reader.read_u32();
                    for (std::uint32_t st = 0; st < string_count; ++st) {
                        std::string key = reader.read_string();
                        eff.params[key] = reader.read_string();
                    }
                    
                    std::uint32_t bool_count = reader.read_u32();
                    for (std::uint32_t b = 0; b < bool_count; ++b) {
                        std::string key = reader.read_string();
                        eff.params[key] = reader.read_bool();
                    }

                    layer.effects.push_back(std::move(eff));
                }
            }

            // Unified Temporal & Tracking
            layer.temporal.track_bindings = reader.read_vector<spec::TrackBinding>([](BinaryReader& r) {
                return static_cast<TBFReader&>(r).read_track_binding();
            });
            layer.temporal.time_remap = reader.read_time_remap();
            layer.temporal.frame_blend = reader.read_enum<spec::FrameBlendMode>();

            comp.layers.push_back(std::move(layer));
        }


        comp.audio_tracks = reader.read_vector<spec::AudioTrackSpec>([](BinaryReader& r) {
            return static_cast<TBFReader&>(r).read_audio_track();
        });

        scene.compositions.push_back(std::move(comp));
    }

    if (reader.error) { std::cerr << "TBF: Error reading compositions\n"; return std::nullopt; }

    // 6. Property Tracks
    scene.property_tracks = reader.read_vector<CompiledPropertyTrack>([](BinaryReader& r) {
        TBFReader& tr = static_cast<TBFReader&>(r);
        CompiledPropertyTrack track;
        track.node = tr.read_node();
        track.property_id = tr.read_string();
        track.kind = tr.read_enum<CompiledPropertyTrack::Kind>();
        track.constant_value = tr.read_f64();
        track.keyframes = tr.read_vector<CompiledKeyframe>([](BinaryReader& r2) {
            CompiledKeyframe kf;
            kf.time = r2.read_f64(); kf.value = r2.read_f64(); kf.easing = r2.read_u32();
            kf.cx1 = r2.read_f64(); kf.cy1 = r2.read_f64(); kf.cx2 = r2.read_f64(); kf.cy2 = r2.read_f64();
            kf.spring_stiffness = r2.read_f64(); kf.spring_damping = r2.read_f64(); kf.spring_mass = r2.read_f64();
            return kf;
        });
        return track;
    });

    if (reader.error) { std::cerr << "TBF: Error reading property tracks\n"; return std::nullopt; }

    // 7. Expressions
    std::uint32_t expression_count = reader.read_u32();
    if (expression_count > kMaxVectorItems) return std::nullopt;
    for (std::uint32_t i = 0; i < expression_count; ++i) {
        expressions::Bytecode expr;
        expr.instructions = reader.read_vector<expressions::Instruction>([](BinaryReader& r) {
            expressions::Instruction instr;
            instr.op = static_cast<expressions::OpCode>(r.read_u8());
            instr.data = r.read_u32();
            return instr;
        });
        expr.constants = reader.read_vector<double>();
        expr.names = reader.read_vector<std::string>([](BinaryReader& r) {
            return r.read_string();
        });
        scene.expressions.push_back(std::move(expr));
    }

    if (reader.error) { std::cerr << "TBF: Error reading expressions\n"; return std::nullopt; }

    // 8. Assets
    scene.assets = reader.read_vector<AssetSpec>([](BinaryReader& r) {
        AssetSpec a;
        a.id = r.read_string();
        a.path = r.read_string();
        a.type = r.read_string();
        return a;
    });

    if (reader.error) { std::cerr << "TBF: Error reading assets\n"; return std::nullopt; }

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
    auto end = file.tellg();
    if (end <= 0) return std::nullopt;
    std::streamsize size = end;
    file.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> buffer(static_cast<std::size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) return std::nullopt;
    return decode(buffer);
}

std::vector<std::uint8_t> TBFCodec::encode_framebuffer(const renderer2d::Framebuffer& fb) {
    BinaryWriter writer;
    writer.write_u32(fb.width());
    writer.write_u32(fb.height());
    writer.write_vector(fb.pixels());
    return std::move(writer.buffer);
}

std::shared_ptr<renderer2d::Framebuffer> TBFCodec::decode_framebuffer(const std::vector<std::uint8_t>& data) {
    BinaryReader reader{data};
    std::uint32_t w = reader.read_u32();
    std::uint32_t h = reader.read_u32();
    auto pixels = reader.read_vector<float>();
    
    if (reader.error || pixels.size() != static_cast<std::size_t>(w) * h * 4U) {
        return nullptr;
    }
    
    auto fb = std::make_shared<renderer2d::Framebuffer>(w, h);
    fb->mutable_pixels() = std::move(pixels);
    return fb;
}

} // namespace tachyon::runtime