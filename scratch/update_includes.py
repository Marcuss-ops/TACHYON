import os
import re

replacements = {
    # runtime/core
    'tachyon/runtime/core/render_graph.h': 'tachyon/runtime/core/graph/render_graph.h',
    'tachyon/runtime/core/property_graph.h': 'tachyon/runtime/core/data/property_graph.h',
    'tachyon/runtime/core/compiled_scene.h': 'tachyon/runtime/core/data/compiled_scene.h',
    'tachyon/runtime/core/diagnostics.h': 'tachyon/runtime/core/diagnostics/diagnostics.h',
    'tachyon/runtime/core/math_contract.h': 'tachyon/runtime/core/contracts/math_contract.h',
    'tachyon/runtime/core/determinism_contract.h': 'tachyon/runtime/core/contracts/determinism_contract.h',
    'tachyon/runtime/core/data_binding.h': 'tachyon/runtime/core/data/data_binding.h',
    'tachyon/runtime/core/data_snapshot.h': 'tachyon/runtime/core/data/data_snapshot.h',
    'tachyon/runtime/core/tbf_codec.h': 'tachyon/runtime/core/serialization/tbf_codec.h',
    'tachyon/runtime/core/composition_fragment.h': 'tachyon/runtime/core/graph/composition_fragment.h',
    
    # core/spec
    'tachyon/core/spec/scene_spec.h': 'tachyon/core/spec/schema/scene_spec.h',
    'tachyon/core/spec/scene_spec_core.h': 'tachyon/core/spec/schema/scene_spec_core.h',
    'tachyon/core/spec/scene_compiler.h': 'tachyon/core/spec/compilation/scene_compiler.h',
    'tachyon/core/spec/layer_spec.h': 'tachyon/core/spec/schema/layer_spec.h',
    'tachyon/core/spec/property_spec.h': 'tachyon/core/spec/schema/property_spec.h',
    'tachyon/core/spec/common_spec.h': 'tachyon/core/spec/schema/common_spec.h',
    'tachyon/core/spec/composition_spec.h': 'tachyon/core/spec/schema/composition_spec.h',
    'tachyon/core/spec/text_animator_spec.h': 'tachyon/core/spec/schema/text_animator_spec.h',
    'tachyon/core/spec/transform_spec.h': 'tachyon/core/spec/schema/transform_spec.h',
    'tachyon/core/spec/asset_spec.h': 'tachyon/core/spec/assets/asset_spec.h',

    # evaluated_composition
    'tachyon/renderer2d/evaluated_composition/composition_utils.h': 'tachyon/renderer2d/evaluated_composition/utilities/composition_utils.h'
}

def update_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    original_content = content
    for old, new in replacements.items():
        # Handle both "tachyon/..." and <tachyon/...>
        content = content.replace(f'"{old}"', f'"{new}"')
        content = content.replace(f'<{old}>', f'<{new}>')
    
    if content != original_content:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        return True
    return False

root_dir = r'c:\Users\pater\Pyt\Tachyon'
updated_count = 0
for root, dirs, files in os.walk(root_dir):
    # Skip some dirs
    if '.git' in dirs: dirs.remove('.git')
    if 'build' in dirs: dirs.remove('build')
    if 'third_party' in dirs: dirs.remove('third_party')
    
    for file in files:
        if file.endswith(('.h', '.cpp')):
            if update_file(os.path.join(root, file)):
                updated_count += 1
                print(f"Updated {file}")

print(f"Finished. Updated {updated_count} files.")
