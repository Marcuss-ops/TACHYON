#include "preview_window.h"
#include <iostream>
#include <GLFW/glfw3.h>

namespace tachyon::cli {

PreviewWindow::PreviewWindow(int width, int height, const std::string& title) {
    if (!glfwInit()) {
        std::cerr << "[Tachyon] Failed to initialize GLFW\n";
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    
    window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window_) {
        std::cerr << "[Tachyon] Failed to create GLFW window\n";
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // Enable vsync

    init_opengl();
}

PreviewWindow::~PreviewWindow() {
    if (texture_id_ != 0) {
        glDeleteTextures(1, &texture_id_);
    }
    if (window_) {
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
}

void PreviewWindow::init_opengl() {
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}

void PreviewWindow::present(const renderer2d::SurfaceRGBA& surface) {
    if (!window_) return;

    glfwMakeContextCurrent(window_);

    int display_w, display_h;
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Upload texture data
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    
    // OpenGL expects pixels from bottom to top, so we need to flip if Tachyon is top-to-bottom.
    // Assuming Tachyon is top-to-bottom.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface.width(), surface.height(), 0, GL_RGBA, GL_FLOAT, surface.pixels().data());

    // Setup Ortho projection for 2D
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // glOrtho(left, right, bottom, top, zNear, zFar)
    glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0); // Flipped Y for top-left origin

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Draw full-screen quad
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 1.0f);
    glEnd();

    glfwSwapBuffers(window_);
}

bool PreviewWindow::poll_events() {
    if (!window_) return false;
    glfwPollEvents();
    return !glfwWindowShouldClose(window_);
}

} // namespace tachyon::cli
