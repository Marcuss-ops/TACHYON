# Tachyon Preview Lane

Standardized preview tools for fast visual iteration and validation.

## Directory Structure

```
tools/preview/
├── README.md          # This file
├── presets/           # Preset-specific preview scripts
├── fixtures/          # Test fixtures and sample data
└── outputs/           # Preview output (gitignored)

previews/              # Standard preview output location
└── <scene_id>/
    └── <composition_id>/
        ├── frame_0000.png
        ├── frame_0090.png
        ├── contact_sheet.jpg
        └── metadata.json
```

## CLI Usage

### Frame Preview
```bash
tachyon preview frame scene.json --frame 90 --out previews/frame_0090.png
```

### Contact Sheet
```bash
tachyon preview contact-sheet scene.json --every 30 --out previews/contact_sheet.jpg
```

### Text Preview
```bash
tachyon preview text typewriter --text "Hello" --out previews/typewriter/
```

## Available Scripts

- `transition_preview.py`: Lightweight transition math preview using OpenCV and NumPy
- `typewriter_preview.py`: Text animator preview using Pillow and ffmpeg
- `text_3d_helpers_preview.py`: Quick text plus 3D-helper preview using Pillow and ffmpeg

## Rules

- Preview scripts must not introduce new render paths
- Preview must call the same pipeline as the main render
- Only frame range and output format may differ
- Output goes to `previews/` directory (gitignored)
- Scripts are dev-only and do not belong in runtime core

## Dependencies

- Python 3.8+
- OpenCV and NumPy for `transition_preview.py`
- Pillow and ffmpeg for text preview scripts
