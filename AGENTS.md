# Tachyon Build Rules

## Build System

**Ninja is the standard build system for Tachyon.**

All builds must use Ninja generator with CMake. The build directory is `build-ninja`.

### Recommended: Use build.ps1 (easiest)

The `build.ps1` script automatically finds CMake/Ninja in Visual Studio and handles all setup:

```powershell
# Clean build (default: RelWithDebInfo)
.\build.ps1 -RelWithDebInfo -Clean

# Build without cleaning
.\build.ps1 -RelWithDebInfo

# Run tests
.\build.ps1 -RelWithDebInfo -Test
```

### Manual Build (advanced)

If you prefer manual cmake commands, you MUST set up the Visual Studio environment first:

```bash
# Configure (first time only)
cmd /c "call ""C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"" && cmake -G Ninja -B build-ninja -S ."

# Build
cmd /c "call ""C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"" && ninja -C build-ninja TachyonCore"
cmd /c "call ""C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"" && ninja -C build-ninja tachyon"
cmd /c "call ""C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"" && ninja -C build-ninja TachyonTests"
```

**Important**: Direct `cmake` or `ninja` commands will fail with "command not found" unless the VS environment is loaded first. If you get this error, use `.\build.ps1` instead.

### Environment Setup

All manual build commands must be run after setting up the Visual Studio environment:

```bash
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
```

### CMake Configuration

- Generator: `Ninja` (not Visual Studio)
- Build directory: `build-ninja` (not `build`)
- All CMakePresets must use Ninja generator

### Scripts

- `build.ps1` - **Recommended** - Uses Ninja and `build-ninja` directory, auto-finds CMake/Ninja
- `run_all_tests.bat` - Uses `build-ninja` directory for test executables
- All custom build scripts must use Ninja

### Why Ninja

- Faster incremental builds
- Better parallelization
- Simpler build output
- Consistent with CI/CD pipelines

### Troubleshooting

**"cmake is not recognized"**
- Use `.\build.ps1` which automatically finds CMake in Visual Studio installation
- Or run `cmd /c "call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" && cmake ..."`

**Build fails with "epsilon" or unused variable warnings treated as errors**
- Ensure code compiles with `/WX` (warnings as errors)
- Remove unused variables or use `(void)var;` to silence

**CMake not found in PowerShell**
- Run `.\scripts\Enable-DevTools.ps1 -PersistUserPath` to add CMake to PATH permanently
