# Tachyon Visual Preview Lane

This directory contains standardized tools for rapid visual iteration of engine features.

## Rules
- **No Engine Dependencies**: These tools should not link against `TachyonCore` or `TachyonRuntime` directly. They consume output from the CLI or generate specifications.
- **Fast Iteration**: Aim for sub-second visual feedback.
- **Environment**: Use Python 3.10+ with OpenCV, Pillow, or FFmpeg.
- **Output**: All preview assets are saved to `output/preview/`.

## Available Tools
- `transition_preview.py`: Visualizes transition functions over time.
- `lightleak_preview.py`: Overlays procedural leaks on test footage.
- `text_preview.py`: Iterates on text layout and animators.
- `background_preview.py`: Previews background presets.
