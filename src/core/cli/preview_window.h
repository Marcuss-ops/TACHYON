#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include <string>
#include <memory>

struct GLFWwindow;

namespace tachyon::cli {

class PreviewWindow {
public:
    PreviewWindow(int width, int height, const std::string& title);
    ~PreviewWindow();

    // Prevent copying
    PreviewWindow(const PreviewWindow&) = delete;
    PreviewWindow& operator=(const PreviewWindow&) = delete;

    /**
     * @brief Present a Tachyon RGBA surface to the native window.
     * @param surface The rendered frame to display.
     */
    void present(const renderer2d::SurfaceRGBA& surface);

    /**
     * @brief Poll window events (e.g. close, resize).
     * @return true if the window should stay open, false if the user requested to close it.
     */
    bool poll_events();

private:
    GLFWwindow* window_{nullptr};
    unsigned int texture_id_{0};
    unsigned int vao_{0};
    unsigned int vbo_{0};
    unsigned int shader_program_{0};

    void init_opengl();
    void compile_shaders();
};

} // namespace tachyon::cli
