# Enable Visual Studio compiler environment for the current session.
# Caches the env vars to avoid the 20-30s vcvars64.bat startup on every build.
# Cache is invalidated when vcvars64.bat changes (new VS update) or is > 8h old.

param(
    [string]$VSVersion    = "auto",
    [switch]$NoCache,
    [switch]$PersistUserPath
)

$ErrorActionPreference = 'Stop'

# ── Find vcvars64.bat ──────────────────────────────────────────────────────────
$vcvarsCandidates = @()

# Use vswhere if available (fastest)
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    $vswhere = "$env:PROGRAMFILES\Microsoft Visual Studio\Installer\vswhere.exe"
}
if (Test-Path $vswhere) {
    $vsPath = & $vswhere -latest -property installationPath 2>$null
    if ($vsPath) {
        $vcvarsCandidates += "$vsPath\VC\Auxiliary\Build\vcvars64.bat"
    }
}

# Fallback: scan known year-based paths (18 = VS2026, 17 = VS2022, 16 = VS2019)
foreach ($year in @(18, 17, 16)) {
    foreach ($ed in @("Community","Professional","Enterprise","BuildTools")) {
        $vcvarsCandidates += "C:\Program Files\Microsoft Visual Studio\$year\$ed\VC\Auxiliary\Build\vcvars64.bat"
    }
}

$vcvars = $vcvarsCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $vcvars) {
    Write-Error "vcvars64.bat not found. Is Visual Studio installed?"
    exit 1
}

# ── Try cache ─────────────────────────────────────────────────────────────────
$cacheFile = Join-Path $env:TEMP "tachyon_vs_env.json"
$criticalVars = @("PATH","INCLUDE","LIB","LIBPATH","VCINSTALLDIR","VSINSTALLDIR","VCToolsInstallDir")

if (-not $NoCache -and (Test-Path $cacheFile)) {
    $cacheAge    = (Get-Date) - (Get-Item $cacheFile).LastWriteTime
    $vcvarsMtime = (Get-Item $vcvars).LastWriteTime.ToString("o")

    if ($cacheAge.TotalHours -lt 8) {
        try {
            $cache = Get-Content $cacheFile -Raw | ConvertFrom-Json
            if ($cache.vcvars_mtime -eq $vcvarsMtime) {
                foreach ($v in $criticalVars) {
                    if ($cache.vars.$v) { Set-Item -Path "env:$v" -Value $cache.vars.$v }
                }
                Write-Host "  VS env: cache hit ($([int]$cacheAge.TotalMinutes)m old) - $vcvars" -ForegroundColor DarkGray
                exit 0
            }
        } catch { <# corrupt cache, fall through #> }
    }
}

# ── Load fresh from vcvars64.bat ──────────────────────────────────────────────
Write-Host "  VS env: loading from $vcvars ..." -ForegroundColor Yellow

$tempBat = [IO.Path]::GetTempFileName() + ".bat"
"@echo off`r`ncall `"$vcvars`" >nul 2>&1`r`necho __ENV_START__`r`nset`r`necho __ENV_END__" |
    Set-Content -Path $tempBat -Encoding ASCII

$envOutput = cmd /c $tempBat 2>&1
Remove-Item $tempBat -Force

$envVars = @{}
$capture = $false
foreach ($line in $envOutput) {
    if ($line -eq "__ENV_START__") { $capture = $true; continue }
    if ($line -eq "__ENV_END__")   { $capture = $false; continue }
    if ($capture -and $line -match "^(.+?)=(.*)$") { $envVars[$Matches[1]] = $Matches[2] }
}

# Apply and save to cache
$cacheVars = @{}
foreach ($v in $criticalVars) {
    if ($envVars.ContainsKey($v)) {
        Set-Item -Path "env:$v" -Value $envVars[$v]
        $cacheVars[$v] = $envVars[$v]
    }
}

@{ vcvars_mtime = (Get-Item $vcvars).LastWriteTime.ToString("o"); vars = $cacheVars } |
    ConvertTo-Json -Depth 3 | Set-Content $cacheFile -Encoding UTF8

Write-Host "  VS env: loaded and cached - cl.exe ready" -ForegroundColor Green
