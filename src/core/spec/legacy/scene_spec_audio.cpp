#include "tachyon/core/spec/schema/audio/audio_spec.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"

#include <nlohmann/json.hpp>

namespace tachyon {

using json = nlohmann::json;

static bool parse_audio_effect(const json& object, AudioEffectSpec& effect,
                               const std::string& /*path*/, DiagnosticBag& /*diagnostics*/) {
    if (!object.contains("type") || !object.at("type").is_string()) {
        return false;
    }
    effect.type = object.at("type").get<std::string>();

    if (object.contains("start_time") && object.at("start_time").is_number()) {
        effect.start_time = object.at("start_time").get<double>();
    }
    if (object.contains("duration") && object.at("duration").is_number()) {
        effect.duration = object.at("duration").get<double>();
    }
    if (object.contains("gain_db") && object.at("gain_db").is_number()) {
        effect.gain_db = object.at("gain_db").get<float>();
    }
    if (object.contains("cutoff_freq_hz") && object.at("cutoff_freq_hz").is_number()) {
        effect.cutoff_freq_hz = object.at("cutoff_freq_hz").get<float>();
    }
    return true;
}

static bool parse_audio_keyframe(const json& object, animation::Keyframe<float>& kf,
                                  const std::string& /*path*/, DiagnosticBag& /*diagnostics*/) {
    if (!object.contains("time") || !object.at("time").is_number()) return false;
    kf.time = object.at("time").get<double>();

    if (object.contains("value") && object.at("value").is_number()) {
        kf.value = object.at("value").get<float>();
    }

    if (object.contains("interpolation")) {
        const auto& interp = object.at("interpolation");
        if (interp.is_string()) {
            const std::string mode = interp.get<std::string>();
            if (mode == "hold") { kf.out_mode = animation::InterpolationMode::Hold; kf.in_mode = animation::InterpolationMode::Hold; }
            else if (mode == "bezier") { kf.out_mode = animation::InterpolationMode::Bezier; kf.in_mode = animation::InterpolationMode::Bezier; }
            else { kf.out_mode = animation::InterpolationMode::Linear; kf.in_mode = animation::InterpolationMode::Linear; }
        }
    }

    if (object.contains("easing")) {
        kf.easing = parse_easing_preset(object.at("easing"));
    }
    if (object.contains("bezier") && object.at("bezier").is_object()) {
        kf.bezier = parse_bezier(object.at("bezier"));
    }
    return true;
}

AudioTrackSpec parse_audio_track(const json& object, const std::string& path, DiagnosticBag& diagnostics) {
    AudioTrackSpec track;
    read_string(object, "id", track.id);
    read_string(object, "source_path", track.source_path);

    if (object.contains("volume") && object.at("volume").is_number()) {
        track.volume = object.at("volume").get<float>();
    }
    if (object.contains("pan") && object.at("pan").is_number()) {
        track.pan = object.at("pan").get<float>();
    }
    if (object.contains("start_offset_seconds") && object.at("start_offset_seconds").is_number()) {
        track.start_offset_seconds = object.at("start_offset_seconds").get<double>();
    }
    if (object.contains("playback_speed") && object.at("playback_speed").is_number()) {
        track.playback_speed = object.at("playback_speed").get<float>();
    }
    if (object.contains("pitch_shift") && object.at("pitch_shift").is_number()) {
        track.pitch_shift = object.at("pitch_shift").get<float>();
    }
    if (object.contains("pitch_correct") && object.at("pitch_correct").is_boolean()) {
        track.pitch_correct = object.at("pitch_correct").get<bool>();
    }

    if (object.contains("volume_keyframes") && object.at("volume_keyframes").is_array()) {
        const auto& kfs = object.at("volume_keyframes");
        for (std::size_t i = 0; i < kfs.size(); ++i) {
            animation::Keyframe<float> kf;
            if (parse_audio_keyframe(kfs[i], kf, path + ".volume_keyframes[" + std::to_string(i) + "]", diagnostics)) {
                track.volume_keyframes.push_back(kf);
            }
        }
    }

    if (object.contains("pan_keyframes") && object.at("pan_keyframes").is_array()) {
        const auto& kfs = object.at("pan_keyframes");
        for (std::size_t i = 0; i < kfs.size(); ++i) {
            animation::Keyframe<float> kf;
            if (parse_audio_keyframe(kfs[i], kf, path + ".pan_keyframes[" + std::to_string(i) + "]", diagnostics)) {
                track.pan_keyframes.push_back(kf);
            }
        }
    }

    if (object.contains("effects") && object.at("effects").is_array()) {
        const auto& effects = object.at("effects");
        for (std::size_t i = 0; i < effects.size(); ++i) {
            AudioEffectSpec effect;
            if (parse_audio_effect(effects[i], effect, path + ".effects[" + std::to_string(i) + "]", diagnostics)) {
                track.effects.push_back(effect);
            }
        }
    }

    return track;
}

void parse_composition_audio_tracks(CompositionSpec& composition, const json& object,
                                     const std::string& path, DiagnosticBag& diagnostics) {
    if (!object.contains("audio_tracks") || !object.at("audio_tracks").is_array()) {
        return;
    }
    const auto& tracks = object.at("audio_tracks");
    for (std::size_t i = 0; i < tracks.size(); ++i) {
        if (tracks[i].is_object()) {
            composition.audio_tracks.push_back(
                parse_audio_track(tracks[i], path + ".audio_tracks[" + std::to_string(i) + "]", diagnostics));
        }
    }
}

} // namespace tachyon
