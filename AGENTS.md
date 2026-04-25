# Tachyon Build Rules

## Build System

**Ninja is the standard build system for Tachyon.**

All builds must use Ninja generator with CMake. The build directory is `build-ninja`.

### Standard Build Commands

```bash
# Configure (first time only, with VS environment)
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cmake -G Ninja -B build-ninja -S .

# Build
ninja -C build-ninja TachyonCore
ninja -C build-ninja tachyon
ninja -C build-ninja TachyonTests
```

### Environment Setup

All build commands must be run after setting up the Visual Studio environment:

```bash
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
```

### CMake Configuration

- Generator: `Ninja` (not Visual Studio)
- Build directory: `build-ninja` (not `build`)
- All CMakePresets must use Ninja generator

### Scripts

- `build.ps1` - Uses Ninja and `build-ninja` directory
- `run_all_tests.bat` - Uses `build-ninja` directory for test executables
- All custom build scripts must use Ninja

### Why Ninja

- Faster incremental builds
- Better parallelization
- Simpler build output
- Consistent with CI/CD pipelines
