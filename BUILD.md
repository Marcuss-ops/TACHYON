---
Status: Canonical
Last reviewed: 2026-05-06
Owner: Core Team
---

# BUILD.md

## Quick Start

```powershell
.\build.ps1 -Check
```

This performs a fast incremental `TachyonCore` build with `RelWithDebInfo` and skips CMake reconfiguration when the Visual Studio build tree already exists.

## Build Presets

### Fast Check (for development)

```powershell
.\build.ps1 -Check
```

Equivalent to a direct `msbuild` build of `build\src\TachyonCore.vcxproj` with `Configuration=RelWithDebInfo`.

If you are in `cmd.exe`, use:

```bat
build.cmd -Check
```

### Core Only

```powershell
.\build.ps1 -RelWithDebInfo -CoreOnly
```

Builds only the core library, skipping tests and executable.

### Full Build

```powershell
.\build.ps1 -RelWithDebInfo
```

Builds everything: TachyonCore, tachyon executable, and TachyonTests.

### Debug Build

```powershell
.\build.ps1 -Debug
```

### Release Build

```powershell
.\build.ps1 -Release
```

## Testing

### Run All Tests

```powershell
.\build.ps1 -RelWithDebInfo -Test
```

If you are in `cmd.exe`, use:

```bat
run_tests.cmd
```

### Run Targeted Tests

```powershell
.\build.ps1 -RelWithDebInfo -TestFilter Component
```

### Build Tests Only

```powershell
.\build.ps1 -TestsOnly
```

## Cleaning

### Clean Build Directory

```powershell
.\build.ps1 -RelWithDebInfo -Clean
```

### Clean Dependencies (use only when needed)

```powershell
.\build.ps1 -RelWithDebInfo -CleanDeps
```

This removes `.cache/fetchcontent` and `build/`. Only use when dependency configuration changes.

## Quick Syntax Check

```powershell
.\scripts\quick-fix.ps1 -SyntaxCheck src\core\scene\builder.cpp
```

For a fast compile-only loop, prefer `.\build.ps1 -Check` or `.\scripts\quick-fix.ps1`.

## Editor Integration

### VS Code

Use `Ctrl+Shift+B` to access build tasks:
- Tachyon: Check Core
- Tachyon: Build RelWithDebInfo
- Tachyon: Test

### clangd

The project generates `compile_commands.json` automatically. Install the clangd extension in VS Code for:
- Real-time diagnostics
- Code completion
- Go-to-definition
- Include suggestions

Configure `.clangd` is already set up with strict include diagnostics.

## Common Workflow

1. Make changes
2. `.\build.ps1 -Check` (fast incremental core build)
3. Fix errors
4. `.\build.ps1 -RelWithDebInfo -TestFilter MyFeature`
5. `.\build.ps1 -RelWithDebInfo` (full build)
6. Commit

## Build Directories

- `build/` - CMake build directory (generated)
- `.cache/fetchcontent/` - Downloaded dependencies (generated)
- `build/compile_commands.json` - Generated for clangd
