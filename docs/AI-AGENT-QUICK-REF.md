# AI Agent Quick Reference Card

## Start Here
1. Read `AGENTS.md` (rules + workflow)
2. Run `.\scripts\setup-dev.ps1` (one-time setup)
3. Read `BUILD.md` for build commands

## Quick Iteration Cycle
```powershell
# 1. Make small change (1-3 files max)
# 2. Quick check (catches compile errors)
.\build.ps1 -Check -ErrorsOnly

# 3. Fix errors if any
# 4. Run targeted tests
.\build.ps1 -RelWithDebInfo -TestFilter YourFeature

# 5. Full validation before commit
.\scripts\agent-validate.ps1 -Scope "YourFeature"
```

## Build Commands (Cheat Sheet)
| Command | Purpose | Time |
|---------|---------|------|
| `.\build.ps1 -Check` | Quick core build | ~30s |
| `.\build.ps1 -Check -ErrorsOnly` | Same, errors only | ~30s |
| `.\build.ps1 -CoreOnly` | Build TachyonCore | ~1min |
| `.\build.ps1 -TestsOnly` | Build tests | ~30s |
| `cmake --build build-ninja --target HeaderSmokeTests` | Header checks | ~10s |
| `.\build.ps1 -RelWithDebInfo -TestFilter X` | Targeted tests | ~1min |
| `.\build.ps1 -RelWithDebInfo -Test` | All tests | ~2min |
| `.\scripts\agent-validate.ps1 -Scope X` | Full validation | ~3min |

## Scope Validation
```powershell
# Check if you're within scope
.\scripts\scope-validate.ps1 -Scope "Component"

# Known scopes: Component, Scene, Renderer2D, Renderer3D, Text, Audio, Media, Timeline, Tracker, Runtime, Editor, Build
```

## Pre-commit Checklist
- [ ] Read `AGENTS.md` rules
- [ ] Changes are small (1-3 files)
- [ ] Ran `.\build.ps1 -Check` (no errors)
- [ ] Ran targeted tests
- [ ] No inline JSON in headers
- [ ] No forbidden files (obj, pdb, exe, dll)
- [ ] Pre-commit hook installed (`.\scripts\pre-commit.ps1 -Install`)

## Emergency Commands
```powershell
# See what you changed
git diff --name-only

# Undo last change
git checkout -- <file>

# Start over from main
git checkout main && git branch -D <your-branch>
```

## Key Rules (Short Version)
1. **Small changes only** (1-3 files)
2. **No inline JSON** in headers (use .cpp)
3. **Run -Check after every change**
4. **Fix errors immediately**
5. **Don't break the build**
