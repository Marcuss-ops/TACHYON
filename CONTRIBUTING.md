---
Status: Canonical
Last reviewed: 2026-05-06
Owner: Core Team
---

# Contributing to Tachyon

## Quick Start for Contributors

1. **Read the docs**: Start with `AGENTS.md` (for AI agents) or `docs/README.md` (for humans)
2. **Setup**: Run `.\scripts\setup-dev.ps1`
3. **Create branch**: `git checkout -b feature/your-feature-name`
4. **Make changes**: Keep them small and focused
5. **Validate**: Run `.\scripts\agent-validate.ps1 -Scope "YourScope"`
6. **Commit**: Use conventional commit messages
7. **PR**: Open a pull request to `main`

## Branch Naming

| Type | Prefix | Example |
|------|--------|---------|
| Feature | `feature/` | `feature/component-system` |
| Bug fix | `fix/` | `fix/header-include-error` |
| Refactor | `refactor/` | `refactor/scene-parser` |
| Docs | `docs/` | `docs/update-readme` |
| Test | `test/` | `test/component-spec` |

## Commit Messages

Follow conventional commits:

```
<type>(<scope>): <subject>

[optional body]

[optional footer]
```

Examples:
```
feat(core): add component system serialization
fix(build): resolve header include error in composition_spec.h
refactor(spec): move JSON serialization to cpp files
test(core): add component spec unit tests
docs(readme): update AI Coding 2.0 workflow
```

## Build Commands (Quick Reference)

| Command | Purpose |
|---------|---------|
| `.\build.ps1 -Check` | Quick core build (fastest) |
| `.\build.ps1 -Check -ErrorsOnly` | Quick build, errors only |
| `.\build.ps1 -Preset dev-fast` | Build TachyonCore |
| `.\build.ps1 -Preset dev-fast -RunTests -TestFilter Component` | Run targeted tests |
| `.\build.ps1 -Preset dev -RunTests` | Run all tests |
| `.\build.ps1 -Preset dev` | Full build |
| `cmake --build build --preset dev --target HeaderSmokeTests` | Header smoke tests |

## AI Agent Guidelines

If you are an AI agent:
1. **Read `AGENTS.md` first** - contains all rules and workflow
2. Make small, focused changes (1-3 files per task)
3. Run `.\build.ps1 -Check` after every change
4. Fix compile errors immediately
5. Run targeted tests before full builds
6. Do NOT run `-CleanDeps` unless explicitly asked
7. Do NOT modify unrelated files
8. Do NOT introduce new subsystems without permission

### Example Agent Workflow

```powershell
# 1. Read relevant files
# 2. Make minimal changes
# 3. Quick check
.\build.ps1 -Check -ErrorsOnly

# 4. Fix errors if any
# 5. Run targeted tests
.\build.ps1 -Preset dev-fast -RunTests -TestFilter YourFeature

# 6. Full validation
.\scripts\agent-validate.ps1 -Scope "YourFeature"
```

## Code Style

- C++20 standard
- No JSON serialization in headers
- Keep headers lightweight
- Prefer extending existing architecture over duplicating logic

## Pull Request Guidelines

1. **Fill the PR template** (if available)
2. **Link related issues**
3. **Describe changes**: what, why, not how
4. **Attach validation output**: include `.\build.ps1 -Check` and test results
5. **Request review** from maintainers
6. **CI must pass**: Debug, RelWithDebInfo, Release builds

## Do NOT Commit

- `build/` directory
- `.cache/` directory
- `output/` directory
- `*.obj`, `*.pdb`, `*.exe`, `*.dll` files
- Node modules or Python virtual environments

## Getting Help

- Check `docs/README.md` for documentation index
- Read `TACHYON_ENGINEERING_RULES.md` for operational rules
- Open an issue for bugs or feature requests
- See `README.md` for project overview and AI Coding 2.0 workflow
