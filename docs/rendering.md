# Rendering Workflow & CLI Guide

This page outlines the canonical, safe path for executing rendering jobs and exporting video compositions using the Tachyon command line interface.

---

## 1. Safe Rendering Workflow

When exporting procedural graphics or testing visual regressions, follow this baseline workflow:

```
┌───────────────────────────────────────┐
│ 1. Compile 'tachyon' CLI executable   │
└───────────────────┬───────────────────┘
                    ▼
┌───────────────────────────────────────┐
│ 2. Dispatch render preset via CLI     │
└───────────────────┬───────────────────┘
                    ▼
┌───────────────────────────────────────┐
│ 3. Export video directly to output/   │
└───────────────────┬───────────────────┘
                    ▼
┌───────────────────────────────────────┐
│ 4. Verify output frame pixel contents │
└───────────────────────────────────────┘
```

---

## 2. Compilation Target

Compile the unified CLI target using the root script preset:

```bash
# Core incremental build check
./build.ps1 -Check

# Build the main CLI executable
./build.ps1 -Target tachyon
```

Upon successful compilation, the executable binary will be available at:

```
./build/src/RelWithDebInfo/tachyon
```

---

## 3. Command Line Execution

Render compositions directly to encoded video files using the standard CLI options.

### Basic Render
Render the baseline preset composition to a standard H.264 MP4 file:

```bash
./build/src/RelWithDebInfo/tachyon render --preset blank_canvas --out output/blank_canvas.mp4
```

### Specifying Output Profiles
Apply specific output rendering presets (handling dimensions, bitrates, and framerates) using the `--output-preset` flag:

```bash
./build/src/RelWithDebInfo/tachyon render --preset blank_canvas --out output/blank_canvas.mp4 --output-preset youtube_1080p_30
```

### Available CLI Controls
- `--preset <name>`: Specifies the declarative scene preset to load.
- `--out <path>`: Output destination path for the compiled video.
- `--output-preset <name>`: Hardware profile configuration (e.g. `youtube_1080p_30`, `instagram_vertical`).
- `--workers <count>`: Number of concurrent threads to dispatch during batch pipelines.

---

## 4. Visual Verification

To guarantee visual integrity and ensure the output contains actual render data (rather than uniform solid frames):

1. Extract a specific timeline frame from the compiled video using FFmpeg:
   ```bash
   ffmpeg -y -ss 1.0 -i output/blank_canvas.mp4 -frames:v 1 -update 1 output/blank_canvas_frame.png
   ```
2. Inspect `output/blank_canvas_frame.png` using a visual browser or golden test comparator to confirm that textures, layouts, and alpha compositions rendered properly.

> [!IMPORTANT]
> **Operational Rule**: If the rendering output is black or empty, do not change renderer, builder, or compilation flags immediately. Run the targeted visual test suites first (`tests/TachyonGoldenTests`) to narrow down whether the issue resides in asset decoding or compositing pipeline steps.
