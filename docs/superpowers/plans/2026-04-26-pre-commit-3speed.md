# Pre-Commit 3-Speed Validation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the slow (~3 min) pre-commit with a 3-speed system: quick (~15s), normal (~45s), full (pre-push only).

**Architecture:** `agent-validate.ps1` gains a `-Mode quick|normal|full` parameter. Pre-commit hook uses `quick` by default, overridable via `TACHYON_VALIDATE_MODE` env var. A new `pre-push` hook runs `full`. `pre-commit.ps1 -Install` installs both hooks.

**Tech Stack:** PowerShell 5.1, git hooks (sh), `build.ps1 -Verify` (cl.exe /Zs), `build.ps1 -Check` (ninja incremental), `build.ps1 -RelWithDebInfo -Test`

---

### Task 1: Rewrite agent-validate.ps1 with 3 modes

**Files:**
- Modify: `scripts/agent-validate.ps1`

- [ ] **Step 1: Replace the file content**

```powershell
# scripts/agent-validate.ps1
# Usage: .\scripts\agent-validate.ps1 [-Mode quick|normal|full]
# TACHYON_VALIDATE_MODE env var overrides -Mode if set.

param(
    [string]$Mode = "",
    # Legacy compat: -Quick maps to quick mode
    [switch]$Quick
)

$ErrorActionPreference = 'Stop'

if ($Quick -and -not $Mode) { $Mode = 'quick' }
if (-not $Mode -and $env:TACHYON_VALIDATE_MODE) { $Mode = $env:TACHYON_VALIDATE_MODE }
if (-not $Mode) { $Mode = 'quick' }

$Mode = $Mode.ToLower()
if ($Mode -notin @('quick', 'normal', 'full')) {
    Write-Host "Unknown mode '$Mode'. Use: quick, normal, full" -ForegroundColor Red
    exit 1
}

Write-Host "=== Tachyon Validation [mode: $Mode] ===" -ForegroundColor Cyan

# ── shared: forbidden files ────────────────────────────────────────────────
Write-Host "`n[check] Forbidden files..." -ForegroundColor Yellow
$stagedFiles = git diff --cached --name-only 2>$null
$forbidden = @("*.obj", "*.pdb", "*.exe", "*.dll", "*.lib")
foreach ($file in $stagedFiles) {
    foreach ($pattern in $forbidden) {
        if ($file -like $pattern) {
            Write-Host "ERROR: staged forbidden file: $file" -ForegroundColor Red
            exit 1
        }
    }
}

# ── shared: no inline JSON in headers ─────────────────────────────────────
Write-Host "[check] Inline JSON in headers..." -ForegroundColor Yellow
$includeDir = Join-Path $PSScriptRoot "..\include\tachyon"
if (Test-Path $includeDir) {
    $headers = Get-ChildItem -Path $includeDir -Filter "*.h" -Recurse
    foreach ($header in $headers) {
        $content = Get-Content $header.FullName -Raw
        if ($content -match "void to_json\s*\(" -or $content -match "void from_json\s*\(") {
            Write-Host "FAIL: inline JSON in $($header.Name) — move to .cpp" -ForegroundColor Red
            exit 1
        }
    }
}

# ── quick: syntax-only check on staged C++ files ──────────────────────────
$compileDb = Join-Path $PSScriptRoot "..\build-ninja\compile_commands.json"
$hasStagedCpp = ($stagedFiles | Where-Object { $_ -match '\.(cpp|h|hpp)$' }).Count -gt 0

if ($Mode -in @('quick', 'normal', 'full') -and $hasStagedCpp) {
    if (Test-Path $compileDb) {
        Write-Host "[check] Syntax check on staged C++ files..." -ForegroundColor Yellow
        & (Join-Path $PSScriptRoot "..\build.ps1") -Verify
        if ($LASTEXITCODE -ne 0) { Write-Host "Syntax check failed!" -ForegroundColor Red; exit 1 }
    } else {
        Write-Host "[skip] compile_commands.json not found — run .\build.ps1 -Check once to generate it" -ForegroundColor Yellow
    }
}

if ($Mode -eq 'quick') {
    Write-Host "`n=== Validation PASSED [quick] ===" -ForegroundColor Green
    exit 0
}

# ── normal: incremental TachyonCore build ─────────────────────────────────
Write-Host "`n[build] TachyonCore (incremental)..." -ForegroundColor Yellow
& (Join-Path $PSScriptRoot "..\build.ps1") -Check
if ($LASTEXITCODE -ne 0) { Write-Host "Core build failed!" -ForegroundColor Red; exit 1 }

if ($Mode -eq 'normal') {
    Write-Host "`n=== Validation PASSED [normal] ===" -ForegroundColor Green
    exit 0
}

# ── full: all tests ────────────────────────────────────────────────────────
Write-Host "`n[build] All tests..." -ForegroundColor Yellow
& (Join-Path $PSScriptRoot "..\build.ps1") -RelWithDebInfo -TestsOnly
if ($LASTEXITCODE -ne 0) { Write-Host "Test build failed!" -ForegroundColor Red; exit 1 }

Write-Host "`n[run] Tests..." -ForegroundColor Yellow
& (Join-Path $PSScriptRoot "..\build.ps1") -RelWithDebInfo -Test
if ($LASTEXITCODE -ne 0) { Write-Host "Tests failed!" -ForegroundColor Red; exit 1 }

Write-Host "`n=== Validation PASSED [full] ===" -ForegroundColor Green
exit 0
```

- [ ] **Step 2: Verify syntax (no execution)**

```powershell
powershell -ExecutionPolicy Bypass -Command "
  \$null = [System.Management.Automation.Language.Parser]::ParseFile(
    'scripts\agent-validate.ps1', [ref]\$null, [ref]\$null)
  Write-Host 'Syntax OK'
"
```

Expected output: `Syntax OK`

- [ ] **Step 3: Commit**

```bash
git add scripts/agent-validate.ps1
git commit -m "refactor(validate): replace -Quick with -Mode quick|normal|full"
```

---

### Task 2: Update pre-commit hook

**Files:**
- Modify: `.git/hooks/pre-commit`

- [ ] **Step 1: Overwrite the hook**

```bash
cat > .git/hooks/pre-commit << 'EOF'
#!/bin/sh
# Tachyon pre-commit hook
# Override mode: TACHYON_VALIDATE_MODE=normal git commit

MODE="${TACHYON_VALIDATE_MODE:-quick}"
exec powershell -ExecutionPolicy Bypass \
  -File "C:\Users\pater\Pyt\Tachyon\scripts\agent-validate.ps1" \
  -Mode "$MODE"
EOF
```

- [ ] **Step 2: Test quick commit (no C++ staged)**

```bash
git commit --allow-empty -m "test: validate quick mode"
```

Expected: passes in <20s, shows `=== Validation PASSED [quick] ===`

- [ ] **Step 3: Test normal mode override**

```bash
TACHYON_VALIDATE_MODE=normal git commit --allow-empty -m "test: normal mode"
```

Expected: shows `[build] TachyonCore (incremental)...` then `=== Validation PASSED [normal] ===`

Note: `--allow-empty` commits don't change files, delete with `git reset HEAD~2` after testing.

---

### Task 3: Create pre-push hook for full validation

**Files:**
- Create: `.git/hooks/pre-push`

- [ ] **Step 1: Write the hook**

```bash
cat > .git/hooks/pre-push << 'EOF'
#!/bin/sh
# Tachyon pre-push hook — full build + all tests
exec powershell -ExecutionPolicy Bypass \
  -File "C:\Users\pater\Pyt\Tachyon\scripts\agent-validate.ps1" \
  -Mode full
EOF
chmod +x .git/hooks/pre-push
```

- [ ] **Step 2: Verify hook is executable**

```bash
ls -la .git/hooks/pre-push
```

Expected: `-rwxr-xr-x` (or similar with x bit set)

---

### Task 4: Update pre-commit.ps1 -Install to install both hooks

**Files:**
- Modify: `scripts/pre-commit.ps1`

- [ ] **Step 1: Replace the hook installation block**

Find the block starting at line 20 (`$hookContent = @"`) and replace with:

```powershell
if ($Install) {
    Write-Host "Installing git hooks..." -ForegroundColor Cyan
    
    if (-not (Test-Path $hookDir)) {
        Write-Host "Not a git repository!" -ForegroundColor Red
        exit 1
    }
    
    $preCommitContent = @'
#!/bin/sh
# Tachyon pre-commit hook
# Override: TACHYON_VALIDATE_MODE=normal git commit
MODE="${TACHYON_VALIDATE_MODE:-quick}"
exec powershell -ExecutionPolicy Bypass -File "C:\Users\pater\Pyt\Tachyon\scripts\agent-validate.ps1" -Mode "$MODE"
'@
    Set-Content -Path "$hookDir/pre-commit" -Value $preCommitContent -NoNewline
    
    $prePushContent = @'
#!/bin/sh
# Tachyon pre-push hook — full build + all tests
exec powershell -ExecutionPolicy Bypass -File "C:\Users\pater\Pyt\Tachyon\scripts\agent-validate.ps1" -Mode full
'@
    Set-Content -Path "$hookDir/pre-push" -Value $prePushContent -NoNewline
    
    Write-Host "pre-commit hook installed (mode: quick)" -ForegroundColor Green
    Write-Host "pre-push hook installed (mode: full)" -ForegroundColor Green
    Write-Host "`nUsage:" -ForegroundColor Cyan
    Write-Host "  git commit               -> quick (~15s)"
    Write-Host "  TACHYON_VALIDATE_MODE=normal git commit  -> normal (~45s)"
    Write-Host "  git push                 -> full (~3-5min)"
    exit 0
}
```

- [ ] **Step 2: Commit**

```bash
git add scripts/pre-commit.ps1
git commit -m "feat(hooks): install both pre-commit (quick) and pre-push (full) hooks"
```

---

### Task 5: Smoke test the full system

- [ ] **Step 1: Test quick path on a real change**

Touch a non-C++ file and commit:
```bash
echo "# test" >> README.md
git add README.md
git commit -m "test: quick hook on non-cpp change"
```
Expected: <15s, skips syntax check (no C++ staged), passes.

- [ ] **Step 2: Clean up test commit**

```bash
git reset HEAD~1
git checkout README.md
```

- [ ] **Step 3: Update docs reference in AGENTS.md (if present)**

```bash
grep -l "pre-commit\|agent-validate\|-Quick" docs/ AGENTS.md 2>/dev/null
```

If AGENTS.md references `-Quick`, update to `-Mode quick|normal|full`.
