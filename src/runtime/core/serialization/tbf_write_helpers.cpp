#include "tachyon/runtime/core/serialization/tbf_codec.h"
#include "src/runtime/core/serialization/binary_io.h"

namespace tachyon::runtime {

// ---------------------------------------------------------------------------
// Specialized Serializers
// ---------------------------------------------------------------------------

static void write_animation_keyframe(BinaryWriter& writer, const animation::Keyframe<float>& kf) {
    writer.write(kf.time);
    writer.write(kf.value);
    writer.write(kf.in_mode);
    writer.write(kf.out_mode);
    writer.write(kf.easing);
    writer.write(kf.bezier);
    writer.write(kf.spring);
    writer.write(kf.out_tangent_time);
    writer.write(kf.out_tangent_value);
    writer.write(kf.in_tangent_time);
    writer.write(kf.in_tangent_value);
}

void write_node(BinaryWriter& writer, const CompiledNode& node) {
    writer.write(node.node_id);
    writer.write(node.version);
    writer.write(node.topo_index);
    writer.write(node.dirty_mask);
    writer.write(node.type);
    writer.write_vector(node.dependencies);
    writer.write_vector(node.dependents);
}

void write_track_binding(BinaryWriter& writer, const spec::TrackBinding& b) {
    writer.write_string(b.property_path);
    writer.write_string(b.source_id);
    writer.write_string(b.source_track_name);
    writer.write(b.influence);
    writer.write(b.enabled);
}

void write_time_remap(BinaryWriter& writer, const spec::TimeRemapCurve& tr) {
    writer.write(tr.enabled);
    writer.write(tr.mode);
    writer.write_vector(tr.keyframes, [](BinaryWriter& w, const std::pair<float, float>& kf) {
        w.write(kf.first);
        w.write(kf.second);
    });
}

void write_camera_cut(BinaryWriter& writer, const spec::CameraCut& cut) {
    writer.write_string(cut.camera_id);
    writer.write(cut.start_seconds);
    writer.write(cut.end_seconds);
}

void write_audio_effect(BinaryWriter& writer, const spec::AudioEffectSpec& effect) {
    writer.write_string(effect.type);
    writer.write(effect.start_time.has_value());
    if (effect.start_time) writer.write(*effect.start_time);
    writer.write(effect.duration.has_value());
    if (effect.duration) writer.write(*effect.duration);
    writer.write(effect.gain_db.has_value());
    if (effect.gain_db) writer.write(*effect.gain_db);
    writer.write(effect.cutoff_freq_hz.has_value());
    if (effect.cutoff_freq_hz) writer.write(*effect.cutoff_freq_hz);
}

void write_audio_track(BinaryWriter& writer, const spec::AudioTrackSpec& track) {
    writer.write_string(track.id);
    writer.write_string(track.source_path);
    writer.write(track.volume);
    writer.write(track.pan);
    writer.write(track.start_offset_seconds);
    
    writer.write<std::uint32_t>(static_cast<std::uint32_t>(track.volume_keyframes.size()));
    for (const auto& kf : track.volume_keyframes) {
        write_animation_keyframe(writer, kf);
    }
    writer.write<std::uint32_t>(static_cast<std::uint32_t>(track.pan_keyframes.size()));
    for (const auto& kf : track.pan_keyframes) {
        write_animation_keyframe(writer, kf);
    }
    
    writer.write_vector(track.effects, write_audio_effect);

    // Version 4 fields
    writer.write(track.playback_speed);
    writer.write(track.pitch_shift);
    writer.write(track.pitch_correct);
}

} // namespace tachyon::runtime
