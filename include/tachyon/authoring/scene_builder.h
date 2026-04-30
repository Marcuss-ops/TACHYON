#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
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

    LayerBuilder& Expression(const std::string& prop, const std::string& expr) {
        if (prop == "opacity") spec_.opacity_property.expression = expr;
        else if (prop == "position") spec_.transform.position_property.expression = expr;
        else if (prop == "scale") spec_.transform.scale_property.expression = expr;
        else if (prop == "rotation") spec_.transform.rotation_property.expression = expr;
        return *this;
    }

private:
    LayerSpec& spec_;
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

    /// Remotion-like Sequence helper
    CompositionBuilder& Sequence(double from, double duration, const std::function<void(CompositionBuilder&)>& content) {
        double old_offset = m_current_offset;
        m_current_offset += from;
        // In a real implementation, we might also clip 'duration'.
        content(*this);
        m_current_offset = old_offset;
        return *this;
    }

    /// Remotion-like stagger helper
    /// Usage: stagger(0.1, [](int index, CompositionBuilder& b) { ... })
    CompositionBuilder& Stagger(double delay, std::size_t count, const std::function<void(std::size_t, CompositionBuilder&)>& content) {
        double old_offset = m_current_offset;
        for (std::size_t i = 0; i < count; ++i) {
            m_current_offset = old_offset + (static_cast<double>(i) * delay);
            content(i, *this);
        }
        m_current_offset = old_offset;
        return *this;
    }

private:
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

} // namespace tachyon::authoring
