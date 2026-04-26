# BUILD.md

## Quick Start

```powershell
.\build.ps1 -Check
```

This builds only `TachyonCore` with RelWithDebInfo - fastest way to catch compile errors.

## Build Presets

### Fast Check (for development)

```powershell
.\build.ps1 -Check
```

Equivalent to: `cmake --build --preset relwithdebinfo --target TachyonCore`

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

This removes `.cache/fetchcontent` and `build-ninja`. Only use when dependency configuration changes.

## Error-Only Output

```powershell
.\build.ps1 -Check -ErrorsOnly
```

Shows only error lines (matching "error C", "fatal error", "FAILED", "Error:").

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
2. `.\build.ps1 -Check` (fast core build)
3. Fix errors
4. `.\build.ps1 -RelWithDebInfo -TestFilter MyFeature`
5. `.\build.ps1 -RelWithDebInfo` (full build)
6. Commit

## Build Directories

- `build-ninja/` - CMake build directory (generated)
- `.cache/fetchcontent/` - Downloaded dependencies (generated)
- `build-ninja/compile_commands.json` - Generated for clangd
