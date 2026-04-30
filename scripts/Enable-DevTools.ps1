<#
.SYNOPSIS
    Adds cmake and ninja to the current session PATH by scanning common install locations.
.PARAMETER PersistUserPath
    Also persist found paths to the user-level PATH (survives terminal restart).
#>
param([switch]$PersistUserPath)

function Find-Tool([string]$Exe) {
    # Already in PATH?
    $found = Get-Command $Exe -ErrorAction SilentlyContinue
    if ($found) { return (Split-Path $found.Source) }

    $candidates = [System.Collections.Generic.List[string]]::new()

    # WinGet packages (user + machine)
    foreach ($base in @("$env:LOCALAPPDATA\Microsoft\WinGet\Packages", "$env:PROGRAMFILES\WinGet\Packages")) {
        if (Test-Path $base) {
            Get-ChildItem $base -Directory -ErrorAction SilentlyContinue |
                ForEach-Object { $candidates.Add((Join-Path $_.FullName "bin")) }
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
    if (($env:PATH -split ';') -notcontains $Dir) { $env:PATH = "$Dir;$env:PATH" }
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
    Write-Warning "ninja.exe not found.  Run: winget install Ninja-build.Ninja"
    $errors++
}

if ($PersistUserPath) {
    $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
    if (-not $userPath) { $userPath = "" }
    $changed = $false
    foreach ($dir in @($cmakeDir, $ninjaDir)) {
        if ($dir -and ($userPath -split ';') -notcontains $dir) {
            $userPath += ";$dir"; $changed = $true
        }
    }
    if ($changed) {
        [Environment]::SetEnvironmentVariable("Path", $userPath.TrimStart(';'), "User")
        Write-Host "  Paths persisted to user PATH (restart terminal to apply)." -ForegroundColor Yellow
    }
}

if ($errors -gt 0) { exit 1 }
