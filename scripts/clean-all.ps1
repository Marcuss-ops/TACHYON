<#
.SYNOPSIS
    Safely clean Tachyon build artefacts.
.PARAMETER BuildOnly
    Remove only compiled output (build\, out\, tests\output).
    Does NOT touch .cache\fetchcontent (slow to re-download dependencies).
.PARAMETER All
    Also wipe .cache\fetchcontent. Next build will re-download all deps (~5-10min).
.PARAMETER VsEnvCache
    Also delete the VS environment cache ($TEMP\tachyon_vs_env.json).
#>
param(
    [switch]$BuildOnly,
    [switch]$All,
    [switch]$VsEnvCache
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot | Split-Path  # scripts\ -> repo root

function Remove-SafeDir([string]$Path, [string]$Label) {
    if (-not (Test-Path $Path)) { return }
    Write-Host "  Removing $Label ..." -ForegroundColor Yellow
    try {
        Remove-Item $Path -Recurse -Force -ErrorAction Stop
        Write-Host "    OK" -ForegroundColor Green
    } catch {
        # Retry once after brief pause (locked files from a crashed build)
        Start-Sleep -Milliseconds 500
        try {
            Remove-Item $Path -Recurse -Force -ErrorAction Stop
            Write-Host "    OK (retry)" -ForegroundColor Green
        } catch {
            Write-Warning "    Could not fully remove $Path : $_"
            Write-Warning "    Close any processes using the build directory and retry."
        }
    }
}

function Remove-SafeGlob([string]$Dir, [string]$Pattern, [string]$Label) {
    if (-not (Test-Path $Dir)) { return }
    $items = Get-ChildItem $Dir -Filter $Pattern -Recurse -ErrorAction SilentlyContinue
    if (-not $items) { return }
    Write-Host "  Cleaning $Label ..." -ForegroundColor Yellow
    $items | Remove-Item -Force -ErrorAction SilentlyContinue
}

Write-Host "Tachyon Clean" -ForegroundColor Cyan

# ── Always: compiled output ────────────────────────────────────────────────────
Remove-SafeDir  (Join-Path $Root "build")   "build/"
Remove-SafeDir  (Join-Path $Root "out")            "out/"
Remove-SafeGlob (Join-Path $Root "tests\output")  "*.png" "test PNG output"
Remove-SafeGlob (Join-Path $Root "tests\output")  "*.mp4" "test MP4 output"

# Keep the VS solution in build/ for IDE navigation, just clean object files and outputs
$buildSrc = Join-Path $Root "build\src"
if (Test-Path $buildSrc) {
    Remove-SafeGlob $buildSrc "*.obj" "build\src .obj files"
    Remove-SafeGlob $buildSrc "*.pdb" "build\src .pdb files"
    Remove-SafeDir (Join-Path $buildSrc "RelWithDebInfo") "build\src RelWithDebInfo dir"
    Remove-SafeDir (Join-Path $buildSrc "Debug") "build\src Debug dir"
    Remove-SafeDir (Join-Path $buildSrc "Release") "build\src Release dir"
}

$buildTests = Join-Path $Root "build\tests"
if (Test-Path $buildTests) {
    Remove-SafeDir (Join-Path $buildTests "RelWithDebInfo") "build\tests RelWithDebInfo dir"
    Remove-SafeDir (Join-Path $buildTests "Debug") "build\tests Debug dir"
    Remove-SafeDir (Join-Path $buildTests "Release") "build\tests Release dir"
}

# ── VS env cache ───────────────────────────────────────────────────────────────
if ($VsEnvCache -or $All) {
    $cache = "$env:TEMP\tachyon_vs_env.json"
    if (Test-Path $cache) {
        Remove-Item $cache -Force
        Write-Host "  Removed VS env cache" -ForegroundColor Yellow
    }
}

# ── fetchcontent cache (slow deps) ────────────────────────────────────────────
if ($All) {
    Write-Host ""
    Write-Host "  WARNING: wiping .cache/fetchcontent - next build re-downloads all deps." -ForegroundColor Red
    Remove-SafeDir (Join-Path $Root ".cache\fetchcontent") ".cache/fetchcontent"
}

Write-Host "Clean done." -ForegroundColor Green
