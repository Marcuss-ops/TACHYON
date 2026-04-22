import os
import re

mappings = {
    'tachyon/renderer2d/draw_list_builder.h': 'tachyon/renderer2d/raster/draw_list_builder.h',
    'tachyon/renderer2d/draw_list_rasterizer.h': 'tachyon/renderer2d/raster/draw_list_rasterizer.h',
    'tachyon/renderer2d/path_rasterizer.h': 'tachyon/renderer2d/raster/path_rasterizer.h',
    'tachyon/renderer2d/draw_command.h': 'tachyon/renderer2d/raster/draw_command.h',
    'tachyon/renderer2d/rasterizer.h': 'tachyon/renderer2d/raster/rasterizer.h',
    'tachyon/renderer2d/rasterizer_ops.h': 'tachyon/renderer2d/raster/rasterizer_ops.h',
    'tachyon/renderer2d/evaluated_composition_renderer.h': 'tachyon/renderer2d/evaluated_composition/composition_renderer.h'
}

def update_includes(directory):
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith(('.h', '.cpp', '.hpp')):
                filepath = os.path.join(root, file)
                with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                
                original_content = content
                for old, new in mappings.items():
                    content = content.replace(f'#include "{old}"', f'#include "{new}"')
                
                if content != original_content:
                    with open(filepath, 'w', encoding='utf-8') as f:
                        f.write(content)
                    print(f"Updated {filepath}")

update_includes('include')
update_includes('src')
update_includes('tests')
