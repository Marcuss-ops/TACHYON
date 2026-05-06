# Tachyon Preview Lane

This directory contains dev-only preview scripts for fast visual iteration.
The goal is to validate timing, composition, and look without waiting for a full C++ render cycle.

## Scripts

- `transition_preview.py`: lightweight transition math preview using OpenCV and NumPy.
- `typewriter_preview.py`: text animator preview using Pillow and ffmpeg.
- `text_3d_helpers_preview.py`: quick text plus 3D-helper preview using Pillow and ffmpeg.

## Rules

- Preview scripts are dev-only.
- They do not belong in the runtime core.
- They should keep output paths under `output/`.
- They should stay small and focused on authoring feedback, not engine behavior.

## Dependencies

- Python 3.8+
- OpenCV and NumPy for `transition_preview.py`
- Pillow and ffmpeg for the text preview scripts

## Usage

```bash
python tools/preview/transition_preview.py --duration 2.0
python tools/preview/text_3d_helpers_preview.py --text "TEXT + 3D HELPERS"
```
