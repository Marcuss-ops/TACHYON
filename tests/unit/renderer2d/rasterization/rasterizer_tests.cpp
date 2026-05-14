#include "tachyon/renderer2d/raster/rasterizer.h"
#include <iostream>
#include <string>
#include <cmath>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

} // namespace

namespace tachyon::renderer2d {

bool run_rasterizer_tests() {
    g_failures = 0;

    // 1. Test clear command
    {
        SurfaceRGBA surface(100, 100);
        DrawCommand2D clear_cmd;
        clear_cmd.kind = DrawCommandKind::Clear;
        clear_cmd.clear = ClearCommand{Color{255, 0, 0, 255}};
        
        DrawList2D dl;
        dl.commands.push_back(clear_cmd);
        
        Rasterizer::rasterize(surface, dl);
        
        auto pixel = surface.get_pixel(50, 50);
        check_true(pixel.r == 255 && pixel.g == 0 && pixel.b == 0, "clear should fill with red");
    }

    // 2. Test solid rect
    {
        SurfaceRGBA surface(100, 100);
        surface.clear(Color::transparent());
        
        DrawCommand2D rect;
        rect.kind = DrawCommandKind::SolidRect;
        rect.solid_rect = SolidRectCommand{
            RectI{10, 10, 50, 50},
            Color{0, 255, 0, 255},
            1.0f
        };
        
        DrawList2D dl;
        dl.commands.push_back(rect);
        
        Rasterizer::rasterize(surface, dl);
        
        auto inside = surface.get_pixel(30, 30);
        check_true(inside.g == 255, "inside rect should be green");
        
        auto outside = surface.get_pixel(5, 5);
        check_true(outside.a == 0, "outside rect should be transparent");
    }

    // 3. Test alpha blending
    {
        SurfaceRGBA surface(100, 100);
        surface.clear(Color{0, 0, 0, 255}); // Black background
        
        DrawCommand2D rect;
        rect.kind = DrawCommandKind::SolidRect;
        rect.solid_rect = SolidRectCommand{
            RectI{0, 0, 100, 100},
            Color{255, 255, 255, 255},
            0.5f // 50% white
        };
        
        DrawList2D dl;
        dl.commands.push_back(rect);
        
        Rasterizer::rasterize(surface, dl);
        
        auto pixel = surface.get_pixel(50, 50);
        // 0.5 * 255 + 0.5 * 0 = 127.5 -> 127 or 128
        check_true(pixel.r > 120 && pixel.r < 135, "blending should result in gray");
    }

    // 4. Test clipping
    {
        SurfaceRGBA surface(100, 100);
        surface.clear(Color::transparent());
        
        DrawCommand2D rect;
        rect.kind = DrawCommandKind::SolidRect;
        rect.clip = RectI{20, 20, 20, 20};
        rect.solid_rect = SolidRectCommand{
            RectI{0, 0, 100, 100},
            Color{255, 255, 255, 255},
            1.0f
        };
        
        DrawList2D dl;
        dl.commands.push_back(rect);
        
        Rasterizer::rasterize(surface, dl);
        
        check_true(surface.get_pixel(30, 30).r == 255, "inside clip should be white");
        check_true(surface.get_pixel(10, 10).a == 0, "outside clip should be transparent");
    }

    // 5. Test textured rect (basic stub)
    {
        SurfaceRGBA surface(100, 100);
        surface.clear(Color::transparent());
        
        // Create a 2x2 red texture
        auto tex_data = std::make_shared<uint8_t[]>(2 * 2 * 4);
        for(int i=0; i<16; ++i) tex_data[i] = (i%4 == 0) ? 255 : (i%4 == 3 ? 255 : 0);
        
        DrawCommand2D textured;
        textured.kind = DrawCommandKind::TexturedRect;
        textured.textured_rect.emplace(TexturedRectCommand{
            RectI{0, 0, 100, 100},
            TextureHandle{"test", 2, 2, tex_data},
            1.0f
        });
        
        DrawList2D dl;
        dl.commands.push_back(textured);
        
        Rasterizer::rasterize(surface, dl);
        // If rasterizer supports textures, center should be red. 
        // If it's a stub, it shouldn't crash.
    }

    return g_failures == 0;
}

} // namespace tachyon::renderer2d
