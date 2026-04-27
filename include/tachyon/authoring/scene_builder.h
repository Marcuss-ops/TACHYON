#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/schema/contracts/shared_contracts.h"
#include <memory>
#include <string>
#include <vector>

namespace tachyon::authoring {

class LayerBuilder {
public:
    explicit LayerBuilder(LayerSpec& spec) : spec_(spec) {}

    LayerBuilder& Name(std::string name) { spec_.name = std::move(name); return *this; }
    LayerBuilder& Opacity(double val) { spec_.opacity = val; return *this; }
    LayerBuilder& StartTime(double t) { spec_.start_time = t; return *this; }
    LayerBuilder& InOut(double in, double out) { spec_.in_point = in; spec_.out_point = out; return *this; }
    
    LayerBuilder& Position(double x, double y) {
        spec_.transform.position_x = x;
        spec_.transform.position_y = y;
        return *this;
    }

    LayerBuilder& Scale(double x, double y) {
        spec_.transform.scale_x = x;
        spec_.transform.scale_y = y;
        return *this;
    }

    LayerBuilder& Rotation(double deg) {
        spec_.transform.rotation = deg;
        return *this;
    }

    LayerBuilder& TextContent(std::string text) {
        spec_.text_content = std::move(text);
        return *this;
    }

    LayerBuilder& Expression(const std::string& prop, const std::string& expr) {
        if (prop == "opacity") spec_.opacity_property.expression = expr;
        else if (prop == "position") spec_.transform.position_property.expression = expr;
        else if (prop == "scale") spec_.transform.scale_property.expression = expr;
        else if (prop == "rotation") spec_.transform.rotation_property.expression = expr;
        return *this;
    }

    // Loop / Freeze (Remotion-style)
    LayerBuilder& Loop(bool enabled = true) { spec_.loop = enabled; return *this; }
    LayerBuilder& Freeze(int frame) {
        spec_.hold_last_frame = true;
        spec_.out_point = frame;
        return *this;
    }

private:
    LayerSpec& spec_;
};

class CompositionBuilder;  // Forward declaration

class AudioTrackBuilder {
public:
    AudioTrackBuilder(AudioTrackSpec& spec, CompositionBuilder& parent, double fps)
        : spec_(spec), parent_(parent), fps_(fps) {}

    // Quando parte (in secondi o in frame)
    AudioTrackBuilder& AtSeconds(double t) {
        spec_.start_offset_seconds = t;
        return *this;
    }
    AudioTrackBuilder& AtFrame(int frame) {
        spec_.start_offset_seconds = frame / fps_;
        return *this;
    }

    // Trim della sorgente
    AudioTrackBuilder& Trim(double in_sec, double out_sec) {
        spec_.in_point_seconds = in_sec;
        spec_.out_point_seconds = out_sec;
        return *this;
    }
    AudioTrackBuilder& TrimFrames(int in_frame, int out_frame) {
        spec_.in_point_seconds = in_frame / fps_;
        spec_.out_point_seconds = out_frame / fps_;
        return *this;
    }

    // Volume, pan
    AudioTrackBuilder& Volume(float v) { spec_.volume = v; return *this; }
    AudioTrackBuilder& Pan(float p) { spec_.pan = p; return *this; }

    // Fade
    AudioTrackBuilder& FadeIn(double seconds) { spec_.fade_in_duration = seconds; return *this; }
    AudioTrackBuilder& FadeOut(double seconds) { spec_.fade_out_duration = seconds; return *this; }

    // Speed / pitch
    AudioTrackBuilder& Speed(float s, bool pitch_correct = false) {
        spec_.playback_speed = s;
        spec_.pitch_correct = pitch_correct;
        return *this;
    }

    // Normalizzazione LUFS
    AudioTrackBuilder& NormalizeLUFS(float target = -14.0f) {
        spec_.normalize_to_lufs = true;
        spec_.target_lufs = target;
        return *this;
    }

    // Volume animato — keyframe semplice
    AudioTrackBuilder& VolumeAt(double time_sec, float value) {
        animation::Keyframe<float> kf;
        kf.time = time_sec;
        kf.value = value;
        spec_.volume_keyframes.push_back(kf);
        return *this;
    }
    AudioTrackBuilder& VolumeAtFrame(int frame, float value) {
        return VolumeAt(frame / fps_, value);
    }

    // Torna al CompositionBuilder per continuare il chain
    CompositionBuilder& Done() { return parent_; }

private:
    AudioTrackSpec& spec_;
    CompositionBuilder& parent_;
    double fps_;
};

class CompositionBuilder {
public:
    explicit CompositionBuilder(CompositionSpec& spec) : spec_(spec) {}

    CompositionBuilder& Name(std::string name) { spec_.name = std::move(name); return *this; }
    
    LayerBuilder Layer(std::string type, std::string id) {
        LayerSpec layer;
        layer.type = std::move(type);
        layer.id = std::move(id);
        layer.enabled = true;
        layer.visible = true;
        layer.start_time = m_current_offset;
        spec_.layers.push_back(std::move(layer));
        return LayerBuilder(spec_.layers.back());
    }

    /// Aggiunge una traccia audio alla composizione
    AudioTrackBuilder Audio(std::string src, std::string id = "") {
        AudioTrackSpec track;
        track.source_path = std::move(src);
        track.id = id.empty()
            ? ("audio_" + std::to_string(spec_.audio_tracks.size()))
            : std::move(id);
        track.start_offset_seconds = m_current_offset; // eredita l'offset del Sequence corrente
        spec_.audio_tracks.push_back(std::move(track));

        double fps = (spec_.frame_rate.denominator > 0)
            ? static_cast<double>(spec_.frame_rate.numerator) / spec_.frame_rate.denominator
            : 30.0;

        return AudioTrackBuilder(spec_.audio_tracks.back(), *this, fps);
    }

    /// Remotion-like Sequence helper
    CompositionBuilder& Sequence(double from, double duration, const std::function<void(CompositionBuilder&)>& content) {
        double old_offset = m_current_offset;
        m_current_offset += from;
        
        std::size_t start_layer_idx = spec_.layers.size();
        std::size_t start_audio_idx = spec_.audio_tracks.size();
        
        content(*this);
        
        double sequence_end_time = m_current_offset + duration;
        
        // Clip layers added during this Sequence
        for (std::size_t i = start_layer_idx; i < spec_.layers.size(); ++i) {
            auto& layer = spec_.layers[i];
            double layer_end = layer.start_time + (layer.duration.has_value() ? *layer.duration : (layer.out_point - layer.in_point));
            if (layer_end > sequence_end_time) {
                double new_duration = sequence_end_time - layer.start_time;
                if (new_duration < 0) new_duration = 0;
                layer.duration = new_duration;
                layer.out_point = layer.in_point + new_duration;
            }
        }
        
        // Clip audio tracks added during this Sequence
        for (std::size_t i = start_audio_idx; i < spec_.audio_tracks.size(); ++i) {
            auto& track = spec_.audio_tracks[i];
            double track_end = track.start_offset_seconds + (track.out_point_seconds - track.in_point_seconds);
            if (track_end > sequence_end_time) {
                double new_duration = sequence_end_time - track.start_offset_seconds;
                if (new_duration < 0) new_duration = 0;
                track.out_point_seconds = track.in_point_seconds + new_duration;
            }
        }

        m_current_offset = old_offset;
        return *this;
    }

    /// Remotion-like stagger helper
    /// Usage: stagger(0.1, [](int index, CompositionBuilder& b) { ... })
    CompositionBuilder& Stagger(double delay, std::size_t count, const std::function<void(std::size_t, CompositionBuilder&)>& content) {
        double old_offset = m_current_offset;
        for (std::size_t i =0; i < count; ++i) {
            m_current_offset = old_offset + (static_cast<double>(i) * delay);
            content(i, *this);
        }
        m_current_offset = old_offset;
        return *this;
    }

    /// Series: puts sequences one after another automatically.
    /// Each segment receives a builder with updated m_current_offset.
    /// Usage: builder.Series({ [](CompositionBuilder& b){ b.Sequence(0, 30, [](CompositionBuilder& b){ ... }); },
    ///                                 [](CompositionBuilder& b){ b.Sequence(0, 60, [](CompositionBuilder& b){ ... }); } });
    CompositionBuilder& Series(const std::vector<std::function<void(CompositionBuilder&)>>& segments) {
        double base_offset = m_current_offset;
        for (const auto& segment : segments) {
            // Each segment starts where the previous one ended (m_current_offset is updated by Sequence)
            // We pass *this; the segment should use Sequence or layers which update m_current_offset
            segment(*this);
        }
        // Do NOT reset m_current_offset; series advances the timeline
        return *this;
    }

    /// Declare a typed variable for template rendering (e.g., Var<std::string>("name"), Var<Color>("accent"))
    template<typename T>
    CompositionBuilder& Var(const std::string& name) {
        VariableDecl decl;
        decl.name = name;
        decl.type = TypeName<T>::value();
        spec_.variable_decls.push_back(decl);
        return *this;
    }

private:
    /// Type trait to map C++ types to string identifiers
    template<typename T>
    struct TypeName { static std::string value() { return "unknown"; } };

    template<>
    struct TypeName<std::string> { static std::string value() { return "string"; } };

    template<>
    struct TypeName<double> { static std::string value() { return "double"; } };

    template<>
    struct TypeName<float> { static std::string value() { return "float"; } };

    template<>
    struct TypeName<int> { static std::string value() { return "int"; } };

    CompositionSpec& spec_;
    double m_current_offset{0.0};
};

class SceneBuilder {
public:
    SceneBuilder() {
        scene_.version = "1.0";
        scene_.spec_version = "1.0";
    }

    SceneBuilder& Project(std::string id, std::string name) {
        scene_.project.id = std::move(id);
        scene_.project.name = std::move(name);
        return *this;
    }

    /// Remotion-like input props: parameterized data for the scene.
    SceneBuilder& Props(const std::map<std::string, nlohmann::json>& props) {
        scene_.input_props = props;
        return *this;
    }

    CompositionBuilder Composition(std::string id, std::int64_t width, std::int64_t height, double duration) {
        CompositionSpec comp;
        comp.id = std::move(id);
        comp.width = width;
        comp.height = height;
        comp.duration = duration;
        comp.frame_rate = {60, 1};
        scene_.compositions.push_back(std::move(comp));
        return CompositionBuilder(scene_.compositions.back());
    }

    SceneSpec Build() && {
        return std::move(scene_);
    }

private:
    SceneSpec scene_;
};

// --- Helpers ---

inline std::string interpolate(double a, double b, const std::string& t_expr = "t") {
    return "interpolate(" + std::to_string(a) + ", " + std::to_string(b) + ", " + t_expr + ")";
}

inline std::string spring(const std::string& t_expr, double from, double to, double freq, double damping) {
    return "spring(" + t_expr + ", " + std::to_string(from) + ", " + std::to_string(to) + ", " +
           std::to_string(freq) + ", " + std::to_string(damping) + ")";
}

inline std::string lerp(double a, double b, const std::string& t_expr) {
    return interpolate(a, b, t_expr);
}

/// Create a template expression for a variable with optional format specifier
/// Usage: TextVar("score") => "{{score}}"
///        TextVar("score", "000") => "{{score:000}}"
///        TextVar("amount", ",.2f") => "{{amount:,.2f}}"
inline std::string TextVar(const std::string& var_name, const std::string& format_spec = "") {
    if (format_spec.empty()) {
        return "{{" + var_name + "}}";
    }
    return "{{" + var_name + ":" + format_spec + "}}";
}

/// Create a date/time template expression with format specifier
/// Usage: TextDate("%Y-%m-%d") => "{{date:%Y-%m-%d}}"
inline std::string TextDate(const std::string& date_format = "%Y-%m-%d") {
    return "{{date:" + date_format + "}}";
}

} // namespace tachyon::authoring
