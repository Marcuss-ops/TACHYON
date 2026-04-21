#include "tachyon/renderer2d/rasterizer.h"
#include "tachyon/renderer2d/draw_list_rasterizer.h"
#include "tachyon/renderer2d/framebuffer.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

} // namespace

bool run_rasterizer_tests() {
    using namespace tachyon;
    using namespace tachyon::renderer2d;

    {
        Framebuffer fb(100, 100);
        fb.clear(Color::black());

        CPURasterizer::draw_rect(fb, {10, 10, 80, 80, Color::blue()});
        CPURasterizer::draw_rect(fb, {30, 30, 40, 40, {128, 0, 0, 128}});

        Color blended = fb.get_pixel(50, 50);
        check_true(blended.r >= 92 && blended.r <= 94, "Linear-light alpha blending: red channel");
        check_true(blended.b >= 187 && blended.b <= 189, "Linear-light alpha blending: blue channel");
    }

    {
        Framebuffer fb(50, 50);
        fb.clear(Color::black());
        CPURasterizer::draw_rect(fb, {-10, -10, 100, 100, Color::white()});

        check_true(fb.get_pixel(0, 0).r == 255, "Clipping: top-left inside");
        check_true(fb.get_pixel(49, 49).r == 255, "Clipping: bottom-right inside");
    }

    {
        Framebuffer fb(100, 100);
        fb.clear(Color::black());
        CPURasterizer::draw_line(fb, {0, 0, 99, 99, Color::green()});

        check_true(fb.get_pixel(0, 0).g == 255, "Line: start point");
        check_true(fb.get_pixel(50, 50).g == 255, "Line: mid point");
        check_true(fb.get_pixel(99, 99).g == 255, "Line: end point");
    }

    {
        Framebuffer texture(2, 2);
        texture.clear(Color::transparent());
        texture.set_pixel(0, 0, {255, 0, 0, 255});
        texture.set_pixel(1, 0, {0, 255, 0, 255});
        texture.set_pixel(0, 1, {0, 0, 255, 255});
        texture.set_pixel(1, 1, {255, 255, 255, 255});

        Framebuffer fb(8, 8);
        fb.clear(Color::black());
        CPURasterizer::draw_textured_quad(fb, TexturedQuadPrimitive::axis_aligned(2, 2, 4, 4, &texture, Color::white()));

        check_true(fb.get_pixel(2, 2).r == 255, "Textured quad: top-left sample copied");
        check_true(fb.get_pixel(5, 2).g == 255, "Textured quad: top-right sample copied");
        check_true(fb.get_pixel(2, 5).b == 255, "Textured quad: bottom-left sample copied");
        check_true(fb.get_pixel(5, 5).r == 255 && fb.get_pixel(5, 5).g == 255, "Textured quad: bottom-right sample copied");
    }

    {
        Framebuffer texture(2, 2);
        texture.clear(Color::transparent());
        texture.set_pixel(0, 0, {255, 0, 0, 255});
        texture.set_pixel(1, 0, {0, 255, 0, 255});
        texture.set_pixel(0, 1, {0, 0, 255, 255});
        texture.set_pixel(1, 1, {255, 255, 255, 255});

        Framebuffer fb(16, 16);
        fb.clear(Color::black());
        CPURasterizer::draw_textured_quad(
            fb,
            TexturedQuadPrimitive::custom(
                {2.0F, 2.0F, 0.0F, 0.0F},
                {11.0F, 3.0F, 1.0F, 0.0F},
                {12.0F, 12.0F, 1.0F, 1.0F},
                {3.0F, 11.0F, 0.0F, 1.0F},
                &texture,
                Color::white()));

        check_true(fb.get_pixel(3, 3).r == 255, "Triangulated quad: top-left region sampled");
        check_true(fb.get_pixel(10, 4).g == 255, "Triangulated quad: top-right region sampled");
        check_true(fb.get_pixel(4, 10).b == 255, "Triangulated quad: bottom-left region sampled");
        check_true(fb.get_pixel(10, 10).r == 255 && fb.get_pixel(10, 10).g == 255, "Triangulated quad: bottom-right region sampled");
    }

    {
        RenderPlan plan;
        plan.composition.width = 64;
        plan.composition.height = 36;
        plan.composition.layer_count = 3;

        FrameRenderTask task;
        task.frame_number = 12;
        task.cache_key.value = "frame-12";

        Framebuffer texture(2, 2);
        texture.clear(Color::transparent());
        texture.set_pixel(0, 0, {255, 255, 255, 255});
        texture.set_pixel(1, 0, {255, 255, 255, 255});
        texture.set_pixel(0, 1, {255, 255, 255, 255});
        texture.set_pixel(1, 1, {255, 255, 255, 255});

        std::vector<renderer2d::DrawCommand2D> commands;
        renderer2d::DrawCommand2D clear;
        clear.kind = renderer2d::DrawCommandKind::Clear;
        clear.clear.emplace(renderer2d::ClearCommand{{10, 20, 30, 255}});
        commands.push_back(clear);

        renderer2d::DrawCommand2D rect;
        rect.kind = renderer2d::DrawCommandKind::SolidRect;
        rect.solid_rect.emplace(renderer2d::SolidRectCommand{RectI{4, 4, 12, 8}, {0, 0, 255, 255}, 1.0f});
        commands.push_back(rect);

        renderer2d::DrawCommand2D line;
        line.kind = renderer2d::DrawCommandKind::Line;
        line.line.emplace(renderer2d::LineCommand{0, 0, 10, 0, {0, 255, 0, 255}});
        commands.push_back(line);

        renderer2d::DrawCommand2D textured;
        textured.kind = renderer2d::DrawCommandKind::TexturedQuad;
        textured.textured_quad.emplace(renderer2d::TexturedQuadCommand{
            renderer2d::TextureHandle{"test-texture", &texture},
            {20.0F, 10.0F},
            {26.0F, 10.0F},
            {26.0F, 16.0F},
            {20.0F, 16.0F},
            1.0F
        });
        commands.push_back(textured);

        RasterizedFrame2D frame = render_frame_2d(plan, task, commands);
        check_true(frame.surface != nullptr, "Frame renderer returns a surface");
        check_true(frame.estimated_draw_ops == commands.size(), "Frame renderer reports executed command count");
        check_true(frame.surface->width() == 64, "Frame surface width matches render plan");
        check_true(frame.surface->height() == 36, "Frame surface height matches render plan");
        check_true(frame.surface->get_pixel(0, 0).g == 255, "Frame line draw reached the framebuffer");
        check_true(frame.surface->get_pixel(8, 8).b == 255, "Frame rect draw reached the framebuffer");
        check_true(frame.surface->get_pixel(22, 12).r == 255, "Frame textured quad reached the framebuffer");
    }

    {
        RenderPlan plan;
        plan.composition.width = 32;
        plan.composition.height = 32;
        plan.composition.layer_count = 1;

        FrameRenderTask task;
        task.frame_number = 7;
        task.cache_key.value = "frame-clip";

        std::vector<renderer2d::DrawCommand2D> commands;
        renderer2d::DrawCommand2D clear;
        clear.kind = renderer2d::DrawCommandKind::Clear;
        clear.clear.emplace(renderer2d::ClearCommand{Color::black()});
        commands.push_back(clear);

        renderer2d::DrawCommand2D rect;
        rect.kind = renderer2d::DrawCommandKind::SolidRect;
        rect.solid_rect.emplace(renderer2d::SolidRectCommand{RectI{0, 0, 20, 20}, Color::white(), 1.0f});
        rect.clip.emplace(RectI{4, 4, 6, 6});
        commands.push_back(rect);

        RasterizedFrame2D frame = render_frame_2d(plan, task, commands);
        check_true(frame.surface != nullptr, "Clipped frame renderer returns a surface");
        check_true(frame.surface->get_pixel(2, 2).r == 0, "Clip prevents drawing outside the clip rect");
        check_true(frame.surface->get_pixel(5, 5).r == 255, "Clip allows drawing inside the clip rect");
        check_true(frame.surface->save_png("tests/output/frame_renderer_clipped.png"), "Frame renderer surface is savable");
    }

    {
        RenderPlan plan;
        plan.composition.width = 32;
        plan.composition.height = 32;
        plan.composition.layer_count = 2;

        FrameRenderTask task;
        task.frame_number = 21;
        task.cache_key.value = "frame-z-order";

        renderer2d::DrawList2D draw_list;

        renderer2d::DrawCommand2D clear;
        clear.kind = renderer2d::DrawCommandKind::Clear;
        clear.clear.emplace(renderer2d::ClearCommand{Color::black()});
        draw_list.commands.push_back(clear);

        renderer2d::DrawCommand2D top_layer;
        top_layer.kind = renderer2d::DrawCommandKind::SolidRect;
        top_layer.z_order = 10;
        top_layer.solid_rect.emplace(renderer2d::SolidRectCommand{RectI{4, 4, 16, 16}, {255, 0, 0, 255}, 1.0f});
        draw_list.commands.push_back(top_layer);

        renderer2d::DrawCommand2D bottom_layer;
        bottom_layer.kind = renderer2d::DrawCommandKind::SolidRect;
        bottom_layer.z_order = 0;
        bottom_layer.solid_rect.emplace(renderer2d::SolidRectCommand{RectI{4, 4, 16, 16}, {0, 0, 255, 255}, 1.0f});
        draw_list.commands.push_back(bottom_layer);

        RasterizedFrame2D frame = render_draw_list_2d(plan, task, draw_list);
        check_true(frame.surface != nullptr, "Draw list rasterizer returns a surface");
        check_true(frame.surface->get_pixel(8, 8).r == 255, "Lower z-order command should render before the higher z-order command");
        check_true(frame.surface->get_pixel(8, 8).b == 0, "Higher z-order command should be on top");
    }

    {
        Framebuffer fb(800, 450);
        fb.clear({20, 25, 30, 255});

        for (int i = 0; i < 800; i += 100) {
            CPURasterizer::draw_line(fb, {i, 0, i, 449, {50, 55, 60, 255}});
        }

        CPURasterizer::draw_rect(fb, {100, 100, 600, 250, {40, 45, 50, 255}});
        CPURasterizer::draw_line(fb, {100, 100, 700, 100, Color::white()});
        CPURasterizer::draw_rect(fb, {150, 150, 100, 100, {200, 100, 0, 150}});
        CPURasterizer::draw_rect(fb, {200, 200, 100, 100, {0, 150, 200, 150}});

        std::filesystem::path out_dir = "tests/output";
        std::filesystem::create_directories(out_dir);
        fb.save_png(out_dir / "raster_test_composition.png");
    }

    return g_failures == 0;
}
