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

## Build commands

Fast check:

```powershell
.\build.ps1 -Check
```

Core build:

```powershell
.\build.ps1 -RelWithDebInfo -CoreOnly
```

Full build:

```powershell
.\build.ps1 -RelWithDebInfo
```

Tests:

```powershell
.\build.ps1 -RelWithDebInfo -Test
```

Targeted tests:

```powershell
.\build.ps1 -RelWithDebInfo -TestFilter Component
```

Clean deps only when necessary:

```powershell
.\build.ps1 -RelWithDebInfo -CleanDeps
```

Quick error check:

```powershell
.\build.ps1 -Check -ErrorsOnly
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
