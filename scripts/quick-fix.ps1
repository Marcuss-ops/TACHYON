# Quick incremental workflow for TachyonCore.
# Skips CMake and uses the existing Visual Studio build tree directly.

param(
    [string]$SyntaxCheck = "",
    [switch]$ConfigOnly
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot

if ($ConfigOnly) {
    & "$Root\build.ps1" -Preset dev
    exit $LASTEXITCODE
}

if ($SyntaxCheck) {
    & "$PSScriptRoot\enable-vs-env.ps1"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    $File = Resolve-Path $SyntaxCheck
    Write-Host "Syntax check: $File" -ForegroundColor Cyan

    $Includes = @(
        "/I$Root\include",
        "/I$Root\build\_deps\fmt-src\include",
        "/I$Root\build\_deps\spdlog-src\include",
        "/I$Root\build\_deps\utf8cpp-src\source",
        "/I$Root\build\_deps\eigen-src",
        "/I$Root\build\_deps\libsndfile-src\include"
    )

    & cl.exe /nologo /Zs /std:c++20 /EHsc /permissive- /W4 @Includes $File 2>&1
    exit $LASTEXITCODE
}

& "$Root\build.ps1" -Check
exit $LASTEXITCODE
