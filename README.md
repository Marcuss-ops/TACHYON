# TACHYON

TACHYON is a deterministic, headless motion graphics, compositing, and 3D rendering engine for automated and personal video production.

## Core idea

TACHYON is not a browser-based renderer, not a DOM-driven video tool, and not a general-purpose editor.
It is a native engine designed to consume declarative scene specifications and produce encoded video output with minimal overhead.

## Project direction

- C++ first
- Headless and server-side
- Deterministic rendering
- CPU-first architecture
- No browser in the render path
- No JavaScript or TypeScript in the render core
- Hybrid rendering strategy for 2D compositing and offline 3D rendering
- No intermediate frame dumps by default
- Strong scene, camera, compositing, and timeline foundations
- Programmatic, data-driven motion workflows
- Explicit execution, caching, memory, and parallelism models
- Native media decode and encode integration

## Current status

This repository is intentionally starting from first principles.
The current focus is on locking architecture, contracts, and subsystem boundaries before implementation depth grows.

## Local toolchain

If `cmake` or `msbuild` are not visible in a fresh shell, run:

```powershell
.\scripts\Enable-DevTools.ps1 -PersistUserPath
```

This script adds the local CMake and Visual Studio Build Tools locations to the current session and can persist them to the user PATH.

## Build

Tachyon uses **Ninja** as the default build system with **CMakePresets** for standardized configurations. The build script `build.ps1` handles all build operations.

### Quick start

```powershell
# Clean build (RelWithDebInfo, default)
.\build.ps1 -RelWithDebInfo -Clean

# Build without cleaning
.\build.ps1 -RelWithDebInfo

# Run tests
.\build.ps1 -RelWithDebInfo -Test
```

### Build presets

| Preset | Description |
|--------|-------------|
| `Debug` | Debug symbols, no optimization |
| `Release` | Full optimization, no debug symbols |
| `RelWithDebInfo` | Optimization + debug info (default) |
| `RelWithDebInfo-Unity` | Unity build for faster compilation |
| `RelWithDebInfo-LLD` | Use lld-link linker (requires LLVM) |
| `RelWithDebInfo-Fast` | Unity + LLD combined |

### Build flags

| Flag | Description |
|------|-------------|
| `-Debug` | Build Debug preset |
| `-Release` | Build Release preset |
| `-RelWithDebInfo` | Build RelWithDebInfo preset (default) |
| `-Clean` | Delete `build-ninja/` only (preserves `.cache/`) |
| `-CleanDeps` | Delete both `build-ninja/` and `.cache/` |
| `-Test` | Run tests after building |
| `-ShowSccacheStats` | Show sccache hit/miss statistics |

### Examples

```powershell
# Debug build with tests
.\build.ps1 -Debug -Clean -Test

# Release build
.\build.ps1 -Release -Clean

# Check sccache statistics
.\build.ps1 -RelWithDebInfo -ShowSccacheStats

# Full clean (remove dependencies too)
.\build.ps1 -RelWithDebInfo -CleanDeps
```

### Prerequisites

- **CMake 3.25+** (required for CMP0141 and MSVC_DEBUG_INFORMATION_FORMAT)
- **Visual Studio 2022** (Community/Professional/Enterprise) with C++ tools
- **Ninja** (bundled with Visual Studio or install separately)
- **sccache** (optional, for compiler cache)

### Advanced: manual CMake commands

```powershell
# Configure (first time only)
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cmake -G Ninja -B build-ninja -S . --preset relwithdebinfo

# Build
ninja -C build-ninja TachyonCore
ninja -C build-ninja tachyon
ninja -C build-ninja TachyonTests
```

### FreeType PDB fix

The build automatically applies `/Z7` (Embedded debug format) to FreeType to avoid parallel build PDB conflicts (`C1041` errors). This is handled via `CMAKE_MSVC_DEBUG_INFORMATION_FORMAT` and target property overrides in `CMakeLists.txt`.

For detailed build documentation, see [BUILD.md](BUILD.md).

## Documentation

The canonical navigation entry for the documentation set is:

- `docs/README.md`
- `TACHYON_ENGINEERING_RULES.md` — operational rules every contributor should read before changing code

The documentation is organized with a numbered structure so contributors can move from project vision to engine contracts to subsystem behavior in a stable order.

### Documentation layout

- `docs/00-project/` — vision, non-goals, roadmap, MVP scope
- `docs/10-architecture/` — architecture, execution model, determinism, parallelism, scene foundations
- `docs/20-contracts/` — cross-cutting engine contracts such as rendering, surfaces, output, CLI/API, and compatibility
- `docs/30-scene-and-animation/` — scene spec, layers, properties, animation, templates, expressions, and timing
- `docs/40-2d-compositing/` — shapes, masks, effects, text, motion blur, and color-related compositing behavior
- `docs/50-3d/` — cameras, lights, backend strategy, path tracing, and 2D/3D boundaries
- `docs/60-runtime/` — caching, memory policy, quality tiers, profiling, and low-end execution strategy
- `docs/70-media-io/` — asset pipeline, decode/encode, render jobs, and output delivery
- `docs/80-audio/` — audio reactivity and audio-driven animation
- `docs/90-testing/` — canonical scenes and validation-oriented documentation

## Recommended reading order

For a new contributor, the best path is:

1. `docs/README.md`
2. `docs/00-project/vision.md`
3. `docs/00-project/non-goals.md`
4. `docs/10-architecture/architecture.md`
5. `docs/00-project/roadmap.md`
6. `docs/00-project/mvp-v1.md`
7. `TACHYON_ENGINEERING_RULES.md`
8. the relevant contract and subsystem folders for the area being worked on

## Direction summary

TACHYON is a scene engine first, then a compositing and media engine, then a hybrid renderer and encoder.
Specialized render backends may exist, including a CPU-first physically based 3D path, but they remain subordinate to the engine's declarative scene, compositing, and determinism contracts.

At its core, the project should evaluate properties in time, derive explicit render work, choose the appropriate 2D or 3D rendering path, and produce deterministic output that scales with compute and benefits from caching and parallel execution.
