# Rendering

This page explains the safe path for rendering built-in presets.

## Recommended workflow

1. Build the `tachyon` executable.
2. Render a preset directly from the CLI.
3. Write the result into `output/`.
4. Verify the first or middle frame if the output looks flat.

## Build

Use the normal Tachyon build pipeline:

```powershell
.\build.ps1 -Target tachyon
```

The executable is typically available at:

```powershell
.\build\src\RelWithDebInfo\tachyon.exe
```

## Render a preset

Render the baseline preset to an MP4 file:

```powershell
.\build\src\RelWithDebInfo\tachyon.exe render --preset blank_canvas --out output\blank_canvas.mp4
```

## Verify the render

If a video looks black or gray, extract a frame and inspect it:

```powershell
ffmpeg -y -ss 1.0 -i output\blank_canvas.mp4 -frames:v 1 -update 1 output\blank_canvas_frame.png
```

Open the extracted PNG and confirm it has visible structure, not a uniform fill.

## Do not change code for a render-only task

If the task is only to regenerate MP4s, do not edit compiler, runtime, or renderer code.

If the output is incorrect, stop and inspect the render path before making broader changes.
