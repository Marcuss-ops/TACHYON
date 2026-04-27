# AGENTS.md

## Rules

- Always start from a clean branch.
- Do not push directly to main.
- Keep changes small and focused.
- Search existing code before adding new systems.
- Prefer extending existing architecture over duplicating logic.
- Do not modify unrelated files.
- Do not commit generated files.
- Do not commit build-ninja, .cache, output, node_modules, *.obj, *.pdb, *.exe, *.dll.

## Available Scripts

| Script | Purpose |
|--------|---------|
| `.\scripts\setup-dev.ps1` | One-time development environment setup |
| `.\scripts\enable-vs-env.ps1` | Load VS environment into current session (run once per shell) |
| `.\scripts\agent-validate.ps1 -Scope "X"` | Full validation (header + core + targeted + full) |
| `.\scripts\pre-commit.ps1 -Install` | Install pre-commit hook |
| `.\scripts\check-test-naming.ps1` | Check test naming conventions |
| `.\scripts\scope-validate.ps1 -Scope "X"` | Verify only scope files modified |

## Windows Build Friction Fixes

**Problem**: Direct `ninja`/`cl.exe` commands fail because VS environment isn't loaded.

**Solution 1: Use build.ps1 (recommended)**
```powershell
.\build.ps1 -Check          # Builds TachyonCore (auto-loads VS env)
.\build.ps1 -Verify         # Syntax check only (uses cl.exe /Zs)
```

**Solution 2: Load VS environment into session**
```powershell
.\scripts\enable-vs-env.ps1   # Now ninja, cmake, cl.exe work directly
ninja -C build-ninja TachyonCore  # Now works!
```

**Solution 3: Lightweight syntax check**
```powershell
.\build.ps1 -Verify -TestFilter "Component"  # Check files matching pattern
.\build.ps1 -Verify                           # Check modified files only
```

## Build commands

Fast check (syntax check):

```powershell
.\build.ps1 -Check
```

Core build (TachyonCore only):

```powershell
.\build.ps1 -BuildType RelWithDebInfo -CoreOnly
```

Full build:

```powershell
.\build.ps1 -BuildType RelWithDebInfo
```

Tests:

```powershell
.\build.ps1 -BuildType RelWithDebInfo -Test
```

Targeted tests:

```powershell
.\build.ps1 -BuildType RelWithDebInfo -TestFilter Component
```

Clean and rebuild:

```powershell
.\build.ps1 -BuildType RelWithDebInfo -Clean
```

Quick error check:

```powershell
.\build.ps1 -Check -ErrorsOnly
```

Header smoke tests (catches header errors early):

```powershell
cmake --build build-ninja --config RelWithDebInfo --target HeaderSmokeTests
```

Full validation script for agents:

```powershell
.\scripts\agent-validate.ps1 -Scope "Component"
```

## Expected workflow

1. Inspect relevant files.
2. Make minimal changes.
3. Run `.\build.ps1 -Check`.
4. Fix compile errors.
5. Run targeted tests.
6. Run full build only after targeted checks pass.
7. Summarize changed files and validation commands.

## Architecture rules

- Prefer small, local changes.
- Do not introduce new subsystems unless explicitly requested.
- Do not duplicate existing parsers, serializers, registries, caches, or render paths.
- If a feature already exists in another layer, extend it instead of recreating it.
- Keep headers lightweight.
- Avoid inline heavy serialization logic in headers.
- Use `nlohmann::json` only in `.cpp` files, not in headers.
- Do not add inline `to_json`/`from_json` in header files.

## Example prompt for agents

```
Implement Component System serialization.

Scope:
- include/tachyon/core/spec/schema/objects/composition_spec.h
- src/core/spec/scene_spec_parse.cpp
- src/core/spec/scene_spec_serialize.cpp
- tests/unit/core/spec/component_spec_tests.cpp

Rules:
- Do not add inline JSON serialization in headers.
- Use nlohmann::json only in .cpp files.
- Follow existing scene spec parse/serialize patterns.
- Run .\build.ps1 -Check.
- Run component-related tests.
- Do not run CleanDeps unless dependency configuration changed.
```
