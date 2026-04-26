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

Tachyon requires:
- **Visual Studio 2022** (Community/Professional/Enterprise) with C++ desktop development workload
- **CMake** (installed via Visual Studio or standalone)
- **Ninja** (installed via Visual Studio or standalone)

If `cmake` or `ninja` are not visible in a fresh shell, run:

```powershell
.\scripts\Enable-DevTools.ps1 -PersistUserPath
```

This script adds the local CMake and Visual Studio Build Tools locations to the current session and can persist them to the user PATH.

### First-time setup

After cloning the repository, run the development setup script:

```powershell
.\scripts\setup-dev.ps1
```

This will:
- Generate `compile_commands.json` for clangd integration
- Verify clangd configuration
- Create necessary project directories
- Optionally install development tools (with `-InstallTools`)

### CMake location

CMake is typically installed at:
- Visual Studio 2022: `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`
- Visual Studio 2019: `C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`

If cmake is not in PATH but Visual Studio is installed, use `build.ps1` which automatically finds CMake.

## Build

Tachyon uses **Ninja** as the default build system with **CMakePresets** for standardized configurations. The build script `build.ps1` handles all build operations.

### AI Coding 2.0 Workflow

For AI agents and automated development, Tachyon provides a structured workflow:

1. **Read the rules**: Start with `AGENTS.md` for coding rules and workflow
2. **Setup environment**: Run `.\scripts\setup-dev.ps1` (one-time setup)
3. **Quick validation**: Use `.\build.ps1 -Check -ErrorsOnly` for fast compile checks
4. **Header checks**: Run `cmake --build build-ninja --preset relwithdebinfo --target HeaderSmokeTests`
5. **Full validation**: Use `.\scripts\agent-validate.ps1 -Scope "FeatureName"`

Key principles:
- Catch errors early with clangd + `-Check`, not after full builds
- Make small, focused changes
- Run targeted tests before full builds
- See `AGENTS.md` for complete workflow

### Quick start (recommended)

```powershell
# Clean build (RelWithDebInfo, default)
.\build.ps1 -RelWithDebInfo -Clean

# Build without cleaning
.\build.ps1 -RelWithDebInfo

# Run tests
.\build.ps1 -RelWithDebInfo -Test
```

### AI Agent Quick Checks

```powershell
# Fastest build - only TachyonCore (for catching compile errors)
.\build.ps1 -Check

# Same as above, but show errors only
.\build.ps1 -Check -ErrorsOnly

# Build only tests
.\build.ps1 -TestsOnly

# Build only core (explicit)
.\build.ps1 -RelWithDebInfo -CoreOnly
```

### Manual build (advanced)

If you prefer manual cmake commands, you must first set up the Visual Studio environment:

```powershell
# Set up VS environment (REQUIRED for manual commands)
cmd /c 'call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && cmake -G Ninja -B build-ninja -S .'

# Build
cmd /c 'call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && ninja -C build-ninja TachyonCore'
cmd /c 'call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && ninja -C build-ninja tachyon'
cmd /c 'call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && ninja -C build-ninja TachyonTests'
```

**Note**: Direct cmake/ninja commands require the Visual Studio environment to be loaded first. If you get "cmake not recognized", use `.\build.ps1` instead.

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
| `-Check` | Quick build (RelWithDebInfo + TachyonCore only) |
| `-CoreOnly` | Build only TachyonCore |
| `-TestsOnly` | Build only TachyonTests |
| `-ErrorsOnly` | Show only errors (use with `-Check`) |
| `-Clean` | Delete `build-ninja/` only (preserves `.cache/`) |
| `-CleanDeps` | Delete both `build-ninja/` and `.cache/` |
| `-Test` | Run tests after building |
| `-TestFilter` | Run targeted tests (e.g., `-TestFilter Component`) |
| `-ShowSccacheStats` | Show sccache hit/miss statistics |

### Examples

```powershell
# Debug build with tests
.\build.ps1 -Debug -Clean -Test

# Release build
.\build.ps1 -Release -Clean

# Quick check (fastest way to catch compile errors)
.\build.ps1 -Check

# Quick check with errors only
.\build.ps1 -Check -ErrorsOnly

# Run targeted tests
.\build.ps1 -RelWithDebInfo -TestFilter Component

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
- **clangd** (optional, for real-time diagnostics in editors)

After building, `compile_commands.json` is generated in `build-ninja/` for use with clangd and other tools.

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

### clangd Integration

Tachyon generates `compile_commands.json` automatically (via `CMAKE_EXPORT_COMPILE_COMMANDS: ON`).

To use clangd in VS Code:
1. Install the **clangd extension** from VS Code marketplace
2. Disable the built-in C/C++ extension (or set `"C_Cpp.intelliSenseEngine": "disabled"`)
3. The `.clangd` file is already configured with strict include diagnostics

Benefits:
- Real-time diagnostics while typing (catches errors before building)
- Code completion and go-to-definition
- Include suggestions and unused include detection

### Header Smoke Tests

Tachyon includes header smoke tests that compile critical headers in isolation to catch errors early:

```powershell
# Run all header smoke tests
cmake --build build-ninja --preset relwithdebinfo --target HeaderSmokeTests

# Individual targets:
# HeaderSmokeCompositionSpec
# HeaderSmokeSceneSpec
# HeaderSmokeLayerSpec
# HeaderSmokeRenderJobSpec
```

These tests catch header errors (missing includes, undeclared types) without building the entire project.

### VS Code Integration

VS Code tasks are pre-configured in `.vscode/tasks.json`:
- Press `Ctrl+Shift+B` to access build tasks
- `Tachyon: Check Core` - Quick build (uses `-Check`)
- `Tachyon: Build RelWithDebInfo` - Full build
- `Tachyon: Test` - Run tests

Errors are captured via the `$msCompile` problem matcher and shown in VS Code's Problems panel.

### Agent Validation

For AI agents, use the validation script:

```powershell
# Full validation (header tests + core build + targeted tests + full build)
.\scripts\agent-validate.ps1 -Scope "Component"

# Quick validation (skip full build)
.\scripts\agent-validate.ps1 -Scope "Component" -Quick
```

See `AGENTS.md` for complete rules and workflow guidelines.

For detailed build documentation, see [BUILD.md](BUILD.md).

## Documentation

### For AI Agents (START HERE)

**New to Tachyon? Read these files in order:**

1. **`AGENTS.md`** — Rules, workflow, and build commands for AI agents
2. **`BUILD.md`** — Complete build documentation with examples
3. **`README.md`** (this file) — Project overview and quick reference

### For Human Contributors

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

### AI Agent Files

| File | Purpose |
|------|---------|
| `AGENTS.md` | Rules, workflow, and build commands for AI agents |
| `BUILD.md` | Complete build documentation |
| `.clangd` | clangd configuration (strict diagnostics) |
| `.clang-tidy` | Static analysis configuration |
| `build.ps1` | Build script with `-Check`, `-CoreOnly`, `-TestsOnly`, `-ErrorsOnly` |
| `scripts/agent-validate.ps1` | Validation script for agents |
| `scripts/setup-dev.ps1` | Development environment setup |
| `.vscode/tasks.json` | VS Code build tasks |

## Recommended reading order

### For AI Agents:
1. `AGENTS.md`
2. `BUILD.md`
3. `README.md` (this file)
4. `TACHYON_ENGINEERING_RULES.md`
5. The relevant `docs/` section for the area being worked on

### For Human Contributors:
1. `docs/README.md`
2. `docs/00-project/vision.md`
3. `docs/00-project/non-goals.md`
4. `docs/10-architecture/architecture.md`
5. `docs/00-project/roadmap.md`
6. `docs/00-project/mvp-v1.md`
7. `TACHYON_ENGINEERING_RULES.md`
8. The relevant contract and subsystem folders for the area being worked on

## Direction summary

TACHYON is a scene engine first, then a compositing and media engine, then a hybrid renderer and encoder.
Specialized render backends may exist, including a CPU-first physically based 3D path, but they remain subordinate to the engine's declarative scene, compositing, and determinism contracts.

At its core, the project should evaluate properties in time, derive explicit render work, choose the appropriate 2D or 3D rendering path, and produce deterministic output that scales with compute and benefits from caching and parallel execution.

## AI Coding 2.0

Tachyon embraces an AI-assisted development workflow where:
- Errors are caught early (clangd + `-Check`), not after full builds
- AI agents follow strict rules (`AGENTS.md`) and make small, focused changes
- Validation is tiered: header smoke tests → core build → targeted tests → full build
- See `AGENTS.md` for the complete AI agent workflow

## CI Pipeline

Tachyon includes GitHub Actions CI workflows (`.github/workflows/ci.yml`):
- **Debug build**: Builds TachyonCore in Debug mode
- **RelWithDebInfo build**: Full build with tests
- **Release build**: Release mode validation

All CI builds run header smoke tests and validate the core library before proceeding.
