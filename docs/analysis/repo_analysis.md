# TACHYON Repository Analysis

## Overview
TACHYON is a high-performance, deterministic, headless motion graphics and compositing engine. It is designed to consume declarative scene specifications and produce encoded video output with minimal overhead. The project heavily relies on C++ (C++20 standard) and avoids browser/DOM-based rendering, prioritizing a native, CPU-first, and deterministic architecture.

## Primary Engine Components
The source code under `src/` and headers under `include/` are logically divided into the following primary components:
- **core**: Contains foundational modules like math, animation, properties, camera, and the scene builder (`src/core/scene`). This reflects the project's transition to a C++-first builder architecture.
- **render**: The generic rendering interface/framework.
- **renderer2d**: Specialized subsystem handling 2D compositing and vector shapes.
- **text**: Handling font rendering and glyphs, backed by FreeType and HarfBuzz.
- **media**: Handles native media decoding and encoding integration (utilizes FFmpeg-related dependencies like libavcodec).
- **audio**: Audio reactivity and audio-driven animation capabilities.
- **timeline / tracker**: Manages execution and scene time evaluation.
- **runtime**: Orchestrates job execution, memory, caching, and exporting to outputs.
- **output**: Final delivery of rendered frames.

## Testing Framework
The repository employs **GoogleTest** (`gtest` and `gmock`) for unit testing. The `tests/` directory is well-organized:
- **unit tests** are located in `tests/unit/`, structured to mirror the engine components (e.g., `tests/unit/core`, `tests/unit/text`).
- Tests are built via several distinct targets defined in CMake (e.g., `TachyonCore`, `TachyonTests`, `TachyonSceneTests`, `TachyonRenderTests`), facilitating targeted builds and testing.
- The project provides utility scripts like `check-disabled-tests.ps1` to ensure tests are properly tracked.

## CI Configuration
Continuous Integration is configured via GitHub Actions (`.github/workflows/ci.yml`), running on both Windows and Linux platforms for `main` branch pushes and pull requests.
- **Linux CI (`linux-ci`)**: Uses an Ubuntu runner, installs GCC, Ninja, and FFmpeg/FreeType/HarfBuzz libraries. Runs tests grouped by `smoke`, `core`, and `render` using `ctest`.
- **Windows CI (`windows-ci`)**: Uses a Windows runner, sets up MSVC and Ninja, builds similarly, and runs `smoke`, `core`, and `render` tests via `ctest`.
- CI strictly uses specific CMake presets (`linux-ci`, `windows-ci`) ensuring standardized flags and isolated build environments.

## Build System
- **CMake** (v3.15+) is the primary build system, heavily leveraging `CMakePresets.json` (presets: `dev`, `asan`, `linux-ci`, `windows-ci`, `release`, etc.).
- Wrappers like `build.ps1` (for Windows) and `build.sh` (for Linux) provide friendly interfaces for common commands.
- External dependencies are managed securely via `FetchContent` (or system packages in CI) and grouped neatly in `cmake/deps/`.
