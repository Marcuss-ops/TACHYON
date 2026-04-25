param(
    [string[]]$Targets = @('TachyonCore', 'tachyon', 'TachyonTests'),
    [switch]$Clean,
    [switch]$Reconfigure,
    [switch]$ShowSccacheStats,
    [switch]$Fast,
    [switch]$Safe,
    [string]$CMakeExe,
    [string]$NinjaExe,
    [string]$SccacheExe,
    [switch]$Test,
    [string]$TestFilter,
    [switch]$NoBuild
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if ($Test -and $NoBuild) {
    throw 'Cannot use -Test and -NoBuild together.'
}

if ($Fast -and $Safe) {
    throw 'Use either -Fast or -Safe, not both.'
}

$useFastProfile = $Fast -or (-not $Safe)

function ConvertTo-CmdArg {
    param(
        [Parameter(Mandatory)]
        [string]$Value
    )

    return '"' + ($Value -replace '"', '""') + '"'
}

function Get-ResolvedCommandPath {
    param(
        [Parameter(Mandatory)]
        [object]$Command
    )

    foreach ($propertyName in @('Path', 'Source', 'Definition')) {
        $property = $Command.PSObject.Properties[$propertyName]
        if ($property -and -not [string]::IsNullOrWhiteSpace([string]$property.Value)) {
            return [string]$property.Value
        }
    }

    return $null
}

function Resolve-Executable {
    param(
        [Parameter(Mandatory)]
        [string]$Name,

        [object[]]$Candidates = @()
    )

    foreach ($candidate in $Candidates) {
        if ($null -eq $candidate) {
            continue
        }

        if ($candidate -is [System.Array]) {
            foreach ($nestedCandidate in $candidate) {
                if ([string]::IsNullOrWhiteSpace($nestedCandidate)) {
                    continue
                }

                if (Test-Path -LiteralPath $nestedCandidate) {
                    return (Resolve-Path -LiteralPath $nestedCandidate).Path
                }

                $command = Get-Command $nestedCandidate -ErrorAction SilentlyContinue
                if ($command) {
                    $resolvedPath = Get-ResolvedCommandPath -Command $command
                    if ($resolvedPath) {
                        return $resolvedPath
                    }
                }

                if (-not ($nestedCandidate -match '[\\/]')) {
                    $whereOutput = & where.exe $nestedCandidate 2>$null
                    if ($LASTEXITCODE -eq 0 -and $whereOutput) {
                        return ($whereOutput | Select-Object -First 1)
                    }
                }
            }
            continue
        }

        if ([string]::IsNullOrWhiteSpace($candidate)) {
            continue
        }

        if (Test-Path -LiteralPath $candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }

        $command = Get-Command $candidate -ErrorAction SilentlyContinue
        if ($command) {
            $resolvedPath = Get-ResolvedCommandPath -Command $command
            if ($resolvedPath) {
                return $resolvedPath
            }
        }

        if (-not ($candidate -match '[\\/]')) {
            $whereOutput = & where.exe $candidate 2>$null
            if ($LASTEXITCODE -eq 0 -and $whereOutput) {
                return ($whereOutput | Select-Object -First 1)
            }
        }
    }

    throw "Unable to locate $Name. Install it or add it to PATH."
}

function Get-CommonExecutableCandidates {
    param(
        [Parameter(Mandatory)]
        [string]$Name
    )

    $candidates = New-Object System.Collections.Generic.List[string]

    switch ($Name) {
        'cmake' {
            $candidates.Add('C:\Program Files\CMake\bin\cmake.exe')
            $candidates.Add('C:\Program Files (x86)\CMake\bin\cmake.exe')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\17\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\17\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
        }
        'ninja' {
            $candidates.Add('C:\Program Files\Ninja\ninja.exe')
            $candidates.Add('C:\Program Files (x86)\Ninja\ninja.exe')
            $candidates.Add('C:\Program Files\CMake\bin\ninja.exe')
            $candidates.Add('C:\Program Files (x86)\CMake\bin\ninja.exe')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\17\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\17\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe')
        }
        'rc' {
            $candidates.Add('C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\rc.exe')
            $candidates.Add('C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x86\rc.exe')
            $candidates.Add('C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\rc.exe')
            $candidates.Add('C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x86\rc.exe')
            $candidates.Add('C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\rc.exe')
            $candidates.Add('C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x86\rc.exe')
        }
        'vcvars64.bat' {
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\17\Community\VC\Auxiliary\Build\vcvars64.bat')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\17\BuildTools\VC\Auxiliary\Build\vcvars64.bat')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat')
        }
    }

    $roots = @(
        'C:\Program Files\Microsoft Visual Studio',
        'C:\Program Files (x86)\Microsoft Visual Studio',
        'C:\Program Files\CMake',
        'C:\Program Files (x86)\CMake',
        'C:\Program Files (x86)\Windows Kits\10\bin',
        'C:\Program Files\Ninja',
        'C:\Program Files (x86)\Ninja'
    )

    $fileName = if ($Name -eq 'vcvars64.bat') { 'vcvars64.bat' } elseif ($Name -eq 'cmake') { 'cmake.exe' } elseif ($Name -eq 'rc') { 'rc.exe' } else { 'ninja.exe' }

    foreach ($root in $roots) {
        if (Test-Path -LiteralPath $root) {
            $found = Get-ChildItem -Path $root -Filter $fileName -Recurse -File -ErrorAction SilentlyContinue |
                Select-Object -ExpandProperty FullName -ErrorAction SilentlyContinue

            foreach ($path in $found) {
                if ($path) {
                    $candidates.Add($path)
                }
            }
        }
    }

    return $candidates.ToArray()
}

function Invoke-WithVcvars {
    param(
        [Parameter(Mandatory)]
        [string]$Vcvars,

        [Parameter(Mandatory)]
        [string]$Executable,

        [Parameter(Mandatory)]
        [string[]]$Arguments
    )

    $commandParts = @(
        'call',
        (ConvertTo-CmdArg $Vcvars),
        '>nul',
        '&&',
        (ConvertTo-CmdArg $Executable)
    )

    foreach ($argument in $Arguments) {
        $commandParts += (ConvertTo-CmdArg $argument)
    }

    $cmdLine = $commandParts -join ' '
    & cmd.exe /d /s /c $cmdLine

    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $($LASTEXITCODE): $Executable"
    }
}

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $repoRoot 'build-ninja'

$vcvars = Resolve-Executable -Name 'vcvars64.bat' -Candidates @(
    @(Get-CommonExecutableCandidates -Name 'vcvars64.bat')
)

$cmake = if ($CMakeExe) { Resolve-Executable -Name 'cmake' -Candidates @($CMakeExe) } else { Resolve-Executable -Name 'cmake' -Candidates @((Get-Command cmake -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue), @(Get-CommonExecutableCandidates -Name 'cmake')) }
$ninja = if ($NinjaExe) { Resolve-Executable -Name 'ninja' -Candidates @($NinjaExe) } else { Resolve-Executable -Name 'ninja' -Candidates @((Get-Command ninja -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue), @(Get-CommonExecutableCandidates -Name 'ninja')) }
$rc = Resolve-Executable -Name 'rc' -Candidates @((Get-CommonExecutableCandidates -Name 'rc'))

$sccache = $null
if ($SccacheExe) {
    $sccache = Resolve-Executable -Name 'sccache' -Candidates @($SccacheExe)
} else {
    $sccacheCandidates = @(
        (Get-Command sccache -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue)
    )

    if ($env:LOCALAPPDATA) {
        $sccacheCandidates += Get-ChildItem -Path (Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Packages\Mozilla.sccache_Microsoft.Winget.Source_8wekyb3d8bbwe') -Filter sccache.exe -Recurse -File -ErrorAction SilentlyContinue |
            Sort-Object FullName -Descending |
            Select-Object -ExpandProperty FullName
    }

    try {
        $sccache = Resolve-Executable -Name 'sccache' -Candidates $sccacheCandidates
    } catch {
        $sccache = $null
    }
}

Write-Host "vcvars: $vcvars"
Write-Host "cmake : $cmake"
Write-Host "ninja : $ninja"
Write-Host "rc    : $rc"
if ($sccache) {
    Write-Host "sccache: $sccache"
}

if ($Clean -and (Test-Path -LiteralPath $buildDir)) {
    Remove-Item -Recurse -Force -LiteralPath $buildDir
}

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

$configureNeeded = $Reconfigure -or -not (Test-Path -LiteralPath (Join-Path $buildDir 'CMakeCache.txt'))
if ($configureNeeded) {
    $ninjaArg = ($ninja -replace '\\', '/')
    $rcArg = ($rc -replace '\\', '/')
    $cmakeArgs = @(
        '-G', 'Ninja',
        '-B', $buildDir,
        '-S', $repoRoot,
        "-DCMAKE_MAKE_PROGRAM=$ninjaArg",
        "-DCMAKE_RC_COMPILER=$rcArg"
    )

    if ($useFastProfile) {
        $cmakeArgs += '-DTACHYON_UNITY_BUILD=ON'
    } else {
        $cmakeArgs += '-DTACHYON_UNITY_BUILD=OFF'
    }

    if ($useFastProfile -and $sccache) {
        $sccacheArg = ($sccache -replace '\\', '/')
        $cmakeArgs += '-DTACHYON_ENABLE_COMPILER_CACHE=ON'
        $cmakeArgs += "-DTACHYON_COMPILER_LAUNCHER=$sccacheArg"
    } else {
        $cmakeArgs += '-DTACHYON_ENABLE_COMPILER_CACHE=OFF'
        $cmakeArgs += '-DTACHYON_COMPILER_LAUNCHER='
    }

    Invoke-WithVcvars -Vcvars $vcvars -Executable $cmake -Arguments $cmakeArgs
}

if ($sccache) {
    & $sccache --zero-stats | Out-Null
}

if (-not $NoBuild) {
    $ninjaArgs = @('-C', $buildDir) + $Targets
    Invoke-WithVcvars -Vcvars $vcvars -Executable $ninja -Arguments $ninjaArgs

    if ($ShowSccacheStats -and $sccache) {
        & $sccache --show-stats
    }
} else {
    Write-Host "Skipping build due to -NoBuild."
}

# Run tests if requested
if ($Test) {
    $testExe = Join-Path $buildDir 'tests' 'TachyonTests.exe'
    if (-not (Test-Path $testExe)) {
        if ($NoBuild) {
            throw "Test executable not found at $testExe and -NoBuild specified."
        }
        Write-Host "Test executable not found. Building TachyonTests..."
        $testTargets = @('TachyonTests')
        $ninjaArgs = @('-C', $buildDir) + $testTargets
        Invoke-WithVcvars -Vcvars $vcvars -Executable $ninja -Arguments $ninjaArgs
    }
    $srcDir = Join-Path $buildDir 'src'
    $testsDir = Join-Path $buildDir 'tests'
    $env:PATH = "$srcDir;$testsDir;" + $env:PATH
    $testArgs = @()
    if ($TestFilter) {
        $testArgs += "--gtest_filter=$TestFilter"
    }
    Write-Host "Running tests..."
    & $testExe $testArgs
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0) {
        throw "Tests failed with exit code $exitCode"
    }
}
