<#
.SYNOPSIS
    Adds cmake, ninja, and sccache to the current session PATH by scanning common install locations.
.PARAMETER PersistUserPath
    Also persist found paths to the user-level PATH (survives terminal restart).
#>
param([switch]$PersistUserPath)

function Find-Tool([string]$Exe) {
    # Already in PATH? (Ignore Android Sdk paths as they are non-preferred/slower)
    $found = Get-Command $Exe -ErrorAction SilentlyContinue
    if ($found) {
        $foundPath = Split-Path $found.Source
        if ($foundPath -notmatch "Android\\Sdk") {
            return $foundPath
        }
    }

    $candidates = [System.Collections.Generic.List[string]]::new()

    # Visual Studio 2022 Native Paths (vswhere discovery)
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) {
        $vswhere = "$env:PROGRAMFILES\Microsoft Visual Studio\Installer\vswhere.exe"
    }
    if (Test-Path $vswhere) {
        $vsPath = & $vswhere -version "[17,18)" -products * -property installationPath 2>$null
        if ($vsPath) {
            if ($Exe -eq "cmake.exe") {
                $candidates.Add("$vsPath\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin")
            } elseif ($Exe -eq "ninja.exe") {
                $candidates.Add("$vsPath\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja")
            }
        }
    }

    # VS Fallback Paths (in case vswhere doesn't return installation path)
    foreach ($year in @(2022, 2019)) {
        foreach ($ed in @("BuildTools", "Community", "Professional", "Enterprise")) {
            $baseVs = "C:\Program Files\Microsoft Visual Studio\$year\$ed"
            $baseVsX86 = "C:\Program Files (x86)\Microsoft Visual Studio\$year\$ed"
            foreach ($base in @($baseVs, $baseVsX86)) {
                if (Test-Path $base) {
                    if ($Exe -eq "cmake.exe") {
                        $candidates.Add("$base\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin")
                    } elseif ($Exe -eq "ninja.exe") {
                        $candidates.Add("$base\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja")
                    }
                }
            }
        }
    }

    # WinGet packages (user + machine)
    foreach ($base in @("$env:LOCALAPPDATA\Microsoft\WinGet\Packages", "$env:PROGRAMFILES\WinGet\Packages")) {
        if (Test-Path $base) {
            # Check standard bin folders
            Get-ChildItem $base -Directory -ErrorAction SilentlyContinue |
                ForEach-Object { $candidates.Add((Join-Path $_.FullName "bin")) }
            
            # Check directories containing the target Exe directly or 1 level deep (e.g. sccache)
            Get-ChildItem $base -Directory -ErrorAction SilentlyContinue | ForEach-Object {
                if (Test-Path (Join-Path $_.FullName $Exe)) {
                    $candidates.Add($_.FullName)
                }
                Get-ChildItem $_.FullName -Directory -ErrorAction SilentlyContinue | ForEach-Object {
                    if (Test-Path (Join-Path $_.FullName $Exe)) {
                        $candidates.Add($_.FullName)
                    }
                }
            }
        }
    }

    # Scoop
    $scoopBase = if ($env:SCOOP) { $env:SCOOP } else { "$env:USERPROFILE\scoop\apps" }
    if (Test-Path $scoopBase) {
        Get-ChildItem $scoopBase -Directory -ErrorAction SilentlyContinue |
            ForEach-Object { $candidates.Add((Join-Path $_.FullName "current\bin")) }
    }

    # Chocolatey
    $candidates.Add("$env:ChocolateyInstall\bin")
    $candidates.Add("C:\ProgramData\chocolatey\bin")

    # cmake-X.Y.Z-windows-* pattern in Program Files and user home
    foreach ($base in @("C:\Program Files", "$env:USERPROFILE")) {
        if (Test-Path $base) {
            Get-ChildItem $base -Directory -Filter "cmake-*" -ErrorAction SilentlyContinue |
                Sort-Object Name -Descending |
                ForEach-Object { $candidates.Add((Join-Path $_.FullName "bin")) }
        }
    }

    # Standard cmake installer
    $candidates.Add("C:\Program Files\CMake\bin")
    $candidates.Add("C:\Program Files (x86)\CMake\bin")

    foreach ($dir in $candidates) {
        if ($dir -and (Test-Path (Join-Path $dir $Exe))) { return $dir }
    }
    return $null
}

function Add-ToPath([string]$Dir) {
    if (-not $Dir) { return }
    # Prepend path to ensure it overrides and bypasses any other/slower versions in the PATH
    $current = $env:PATH -split ';'
    $filtered = $current | Where-Object { $_ -and ($_ -ne $Dir) -and ($_ -notmatch "Android\\Sdk") }
    $env:PATH = ($Dir, $filtered) -join ';'
}

$errors = 0

$cmakeDir = Find-Tool "cmake.exe"
if ($cmakeDir) {
    Add-ToPath $cmakeDir
    Write-Host "  cmake : $cmakeDir" -ForegroundColor Green
} else {
    Write-Warning "cmake.exe not found. Install from https://cmake.org/download/"
    $errors++
}

$ninjaDir = Find-Tool "ninja.exe"
if ($ninjaDir) {
    Add-ToPath $ninjaDir
    Write-Host "  ninja : $ninjaDir" -ForegroundColor Green
} else {
    Write-Warning "ninja.exe not found. Run: winget install Ninja-build.Ninja"
    $errors++
}

$sccacheDir = Find-Tool "sccache.exe"
if ($sccacheDir) {
    Add-ToPath $sccacheDir
    Write-Host "  sccache : $sccacheDir" -ForegroundColor Green
} else {
    Write-Warning "sccache.exe not found. Install from winget: winget install Mozilla.sccache"
}

if ($PersistUserPath) {
    $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
    if (-not $userPath) { $userPath = "" }
    $changed = $false
    foreach ($dir in @($cmakeDir, $ninjaDir, $sccacheDir)) {
        if ($dir -and ($userPath -split ';') -notcontains $dir) {
            $userPath += ";$dir"; $changed = $true
        }
    }
    if ($changed) {
        [Environment]::SetEnvironmentVariable("Path", $userPath.TrimStart(';'), "User")
        Write-Host "  Paths persisted to user PATH (restart terminal to apply)." -ForegroundColor Yellow
    }
}

if ($cmakeDir) {
    # Only exit with error if cmake is missing
} else {
    exit 1
}
