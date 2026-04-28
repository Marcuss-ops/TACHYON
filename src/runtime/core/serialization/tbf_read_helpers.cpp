#include "tachyon/runtime/core/serialization/tbf_codec.h"
#include "src/runtime/core/serialization/binary_io.h"

namespace tachyon::runtime {

// ---------------------------------------------------------------------------
// Specialized Deserializers
// ---------------------------------------------------------------------------

static animation::Keyframe<float> read_animation_keyframe(BinaryReader& reader) {
    animation::Keyframe<float> kf;
    kf.time = reader.read<double>();
    kf.value = reader.read<float>();
    kf.in_mode = reader.read<animation::InterpolationMode>();
    kf.out_mode = reader.read<animation::InterpolationMode>();
    kf.easing = reader.read<animation::EasingPreset>();
    kf.bezier = reader.read<animation::CubicBezierEasing>();
    kf.spring = reader.read<animation::SpringEasing>();
    kf.out_tangent_time = reader.read<double>();
    kf.out_tangent_value = reader.read<float>();
    kf.in_tangent_time = reader.read<double>();
    kf.in_tangent_value = reader.read<float>();
    return kf;
}

CompiledNode read_node(BinaryReader& reader) {
    CompiledNode node;
    node.node_id = reader.read<std::uint32_t>();
    node.version = reader.read<std::uint32_t>();
    node.topo_index = reader.read<std::uint32_t>();
    node.dirty_mask = reader.read<std::uint64_t>();
    node.type = reader.read<CompiledNodeType>();
    node.dependencies = reader.read_vector<std::uint32_t>();
    node.dependents = reader.read_vector<std::uint32_t>();
    return node;
}

spec::TrackBinding read_track_binding(BinaryReader& reader) {
    spec::TrackBinding b;
    b.property_path = reader.read_string();
    b.source_id = reader.read_string();
    b.source_track_name = reader.read_string();
    b.influence = reader.read<float>();
    b.enabled = reader.read<bool>();
    return b;
}

spec::TimeRemapCurve read_time_remap(BinaryReader& reader) {
    spec::TimeRemapCurve tr;
    tr.enabled = reader.read<bool>();
    tr.mode = reader.read<spec::TimeRemapMode>();
    tr.keyframes = reader.read_vector<std::pair<float, float>>([](BinaryReader& r) {
        float first = r.read<float>();
        float second = r.read<float>();
        return std::make_pair(first, second);
    });
    return tr;
}

spec::CameraCut read_camera_cut(BinaryReader& reader) {
    spec::CameraCut cut;
    cut.camera_id = reader.read_string();
    cut.start_seconds = reader.read<double>();
    cut.end_seconds = reader.read<double>();
    return cut;
}

spec::AudioEffectSpec read_audio_effect(BinaryReader& reader) {
    spec::AudioEffectSpec effect;
    effect.type = reader.read_string();
    if (reader.read<bool>()) effect.start_time = reader.read<double>();
    if (reader.read<bool>()) effect.duration = reader.read<double>();
    if (reader.read<bool>()) effect.gain_db = reader.read<float>();
    if (reader.read<bool>()) effect.cutoff_freq_hz = reader.read<float>();
    return effect;
}

spec::AudioTrackSpec read_audio_track(BinaryReader& reader) {
    spec::AudioTrackSpec track;
    track.id = reader.read_string();
    track.source_path = reader.read_string();
    track.volume = reader.read<float>();
    track.pan = reader.read<float>();
    track.start_offset_seconds = reader.read<double>();

    const std::uint32_t volume_keyframe_count = reader.read<std::uint32_t>();
    track.volume_keyframes.reserve(volume_keyframe_count);
    for (std::uint32_t i = 0; i < volume_keyframe_count; ++i) {
        track.volume_keyframes.push_back(read_animation_keyframe(reader));
    }

    const std::uint32_t pan_keyframe_count = reader.read<std::uint32_t>();
    track.pan_keyframes.reserve(pan_keyframe_count);
    for (std::uint32_t i = 0; i < pan_keyframe_count; ++i) {
        track.pan_keyframes.push_back(read_animation_keyframe(reader));
    }
    
    track.effects = reader.read_vector<spec::AudioEffectSpec>(read_audio_effect);

    // Version 4 fields
    if (reader.file_version >= 4) {
        track.playback_speed = reader.read<float>();
        track.pitch_shift = reader.read<float>();
        track.pitch_correct = reader.read<bool>();
    }
    return track;
}

} // namespace tachyon::runtime
