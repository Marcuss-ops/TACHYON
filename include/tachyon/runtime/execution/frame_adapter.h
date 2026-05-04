#pragma once

#include "tachyon/core/spec/schema/objects/composition_spec.h"

#include <algorithm>
#include <cstddef>
#include <string>

namespace tachyon::runtime {

struct FrameContext {
    std::string composition_id;
    int width{1920};
    int height{1080};
    double fps{30.0};
    double duration_seconds{0.0};
};

class FrameAdapter {
public:
    virtual ~FrameAdapter() = default;

    [[nodiscard]] virtual std::string id() const = 0;
    virtual void init(const FrameContext& ctx) = 0;
    [[nodiscard]] virtual int duration_frames() const = 0;
    virtual void seek_frame(int frame) = 0;
    virtual void destroy() = 0;
};

class NativeSceneFrameAdapter final : public FrameAdapter {
public:
    explicit NativeSceneFrameAdapter(const CompositionSpec& composition) : m_composition(&composition) {}

    [[nodiscard]] std::string id() const override {
        return "native_scene";
    }

    void init(const FrameContext& ctx) override {
        m_context = ctx;
        m_current_frame = 0;
        m_current_time = 0.0;
    }

    [[nodiscard]] int duration_frames() const override {
        const double fps = m_context.fps > 0.0 ? m_context.fps : m_composition->frame_rate.value();
        if (fps <= 0.0) {
            return 0;
        }
        return static_cast<int>(std::max(0.0, m_context.duration_seconds > 0.0 ? m_context.duration_seconds : m_composition->duration) * fps);
    }

    void seek_frame(int frame) override {
        m_current_frame = std::max(0, frame);
        const double fps = m_context.fps > 0.0 ? m_context.fps : m_composition->frame_rate.value();
        m_current_time = fps > 0.0 ? static_cast<double>(m_current_frame) / fps : 0.0;
    }

    void destroy() override {
        m_current_frame = 0;
        m_current_time = 0.0;
    }

    [[nodiscard]] int current_frame() const noexcept {
        return m_current_frame;
    }

    [[nodiscard]] double current_time_seconds() const noexcept {
        return m_current_time;
    }

private:
    const CompositionSpec* m_composition{nullptr};
    FrameContext m_context{};
    int m_current_frame{0};
    double m_current_time{0.0};
};

} // namespace tachyon::runtime
