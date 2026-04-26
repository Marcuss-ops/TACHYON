param(
    [switch]$Debug,
    [switch]$Release,
    [switch]$RelWithDebInfo,
    [switch]$Check,
    [switch]$CoreOnly,
    [switch]$TestsOnly,
    [switch]$ErrorsOnly,
    [switch]$Clean,
    [switch]$CleanDeps,
    [switch]$Reconfigure,
    [switch]$ShowSccacheStats,
    [switch]$Test,
    [string]$TestFilter,
    [string]$CMakeExe,
    [string]$NinjaExe,
    [string]$SccacheExe
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if ($Debug -and $Release) {
    throw 'Use either -Debug or -Release, not both.'
}

if ($Debug -and $RelWithDebInfo) {
    throw 'Use either -Debug or -RelWithDebInfo, not both.'
}

if ($Release -and $RelWithDebInfo) {
    throw 'Use either -Release or -RelWithDebInfo, not both.'
}

if ($Check) {
    $buildType = 'relwithdebinfo'
    $CoreOnly = $true
} else {
    $buildType = if ($Release) { 'release' } elseif ($RelWithDebInfo) { 'relwithdebinfo' } else { 'debug' }
}

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
        if ($null -eq $candidate) { continue }
        if ($candidate -is [System.Array]) {
            foreach ($nestedCandidate in $candidate) {
                if ([string]::IsNullOrWhiteSpace($nestedCandidate)) { continue }
                if (Test-Path -LiteralPath $nestedCandidate) {
                    return (Resolve-Path -LiteralPath $nestedCandidate).Path
                }
                $command = Get-Command $nestedCandidate -ErrorAction SilentlyContinue
                if ($command) {
                    $resolvedPath = Get-ResolvedCommandPath -Command $command
                    if ($resolvedPath) { return $resolvedPath }
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
        if ([string]::IsNullOrWhiteSpace($candidate)) { continue }
        if (Test-Path -LiteralPath $candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
        $command = Get-Command $candidate -ErrorAction SilentlyContinue
        if ($command) {
            $resolvedPath = Get-ResolvedCommandPath -Command $command
            if ($resolvedPath) { return $resolvedPath }
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
    param([Parameter(Mandatory)][string]$Name)
    $candidates = New-Object System.Collections.Generic.List[string]
    switch ($Name) {
        'cmake' {
            $candidates.Add('C:\Program Files\CMake\bin\cmake.exe')
            $candidates.Add('C:\Program Files (x86)\CMake\bin\cmake.exe')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\17\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
        }
        'ninja' {
            $candidates.Add('C:\Program Files\Ninja\ninja.exe')
            $candidates.Add('C:\Program Files (x86)\Ninja\ninja.exe')
            $candidates.Add('C:\Program Files\CMake\bin\ninja.exe')
            $candidates.Add('C:\Program Files (x86)\CMake\bin\ninja.exe')
        }
        'vcvars64.bat' {
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\17\Community\VC\Auxiliary\Build\vcvars64.bat')
            $candidates.Add('C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat')
        }
    }
    $roots = @(
        'C:\Program Files\Microsoft Visual Studio',
        'C:\Program Files (x86)\Microsoft Visual Studio',
        'C:\Program Files\CMake',
        'C:\Program Files (x86)\CMake',
        'C:\Program Files\Ninja',
        'C:\Program Files (x86)\Ninja'
    )
    foreach ($root in $roots) {
        if (Test-Path -LiteralPath $root) {
            $fileName = switch ($Name) {
                'vcvars64.bat' { 'vcvars64.bat' }
                'cmake' { 'cmake.exe' }
                default { 'ninja.exe' }
            }
            $found = Get-ChildItem -Path $root -Filter $fileName -Recurse -File -ErrorAction SilentlyContinue |
                Select-Object -ExpandProperty FullName -ErrorAction SilentlyContinue
            foreach ($path in $found) {
                if ($path) { $candidates.Add($path) }
            }
        }
    }
    return $candidates.ToArray()
}

function Invoke-WithVcvars {
    param(
        [Parameter(Mandatory)][string]$Vcvars,
        [Parameter(Mandatory)][string]$Executable,
        [Parameter(Mandatory)][string[]]$Arguments,
        [switch]$PassThru
    )
    $commandParts = @('call', (ConvertTo-CmdArg $vcvars), '>nul', '&&', (ConvertTo-CmdArg $Executable))
    foreach ($argument in $Arguments) {
        $commandParts += (ConvertTo-CmdArg $argument)
    }
    $cmdLine = $commandParts -join ' '
    if ($PassThru) {
        $output = & cmd.exe /d /s /c $cmdLine 2>&1
        $output
    } else {
        & cmd.exe /d /s /c $cmdLine
        if ($LASTEXITCODE -ne 0) {
            throw "Command failed with exit code $($LASTEXITCODE): $Executable"
        }
    }
}

# Check for CMake availability early with a clear error message
if (!(Get-Command cmake -ErrorAction SilentlyContinue)) {
    $cmakeCandidates = Get-CommonExecutableCandidates -Name 'cmake'
    $cmakeFound = $false
    foreach ($candidate in $cmakeCandidates) {
        if (Test-Path -LiteralPath $candidate) {
            $cmakeFound = $true
            break
        }
    }
    if (-not $cmakeFound) {
        Write-Error "CMake non trovato. Installa Visual Studio con 'C++ CMake tools' o aggiungi CMake al PATH."
        Write-Host "Percorsi cercati:" -ForegroundColor Yellow
        foreach ($candidate in $cmakeCandidates) {
            Write-Host "  $candidate" -ForegroundColor Yellow
        }
        Write-Host "`nSoluzioni:" -ForegroundColor Green
        Write-Host "  1. Esegui: .\scripts\Enable-DevTools.ps1 -PersistUserPath" -ForegroundColor Green
        Write-Host "  2. Installa Visual Studio con il carico di lavoro 'Sviluppo di applicazioni desktop con C++'" -ForegroundColor Green
        exit 1
    }
}

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $repoRoot 'build-ninja'

$vcvars = Resolve-Executable -Name 'vcvars64.bat' -Candidates @(
    @(Get-CommonExecutableCandidates -Name 'vcvars64.bat')
)

$cmake = if ($CMakeExe) {
    Resolve-Executable -Name 'cmake' -Candidates @($CMakeExe)
} else {
    Resolve-Executable -Name 'cmake' -Candidates @(
        (Get-Command cmake -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue),
        @(Get-CommonExecutableCandidates -Name 'cmake')
    )
}

$ninja = if ($NinjaExe) {
    Resolve-Executable -Name 'ninja' -Candidates @($NinjaExe)
} else {
    Resolve-Executable -Name 'ninja' -Candidates @(
        (Get-Command ninja -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue),
        @(Get-CommonExecutableCandidates -Name 'ninja')
    )
}

$sccache = $null
if ($SccacheExe) {
    $sccache = Resolve-Executable -Name 'sccache' -Candidates @($SccacheExe)
} else {
    $sccacheCandidates = @(
        (Get-Command sccache -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue),
        "$env:USERPROFILE\.cargo\bin\sccache.exe",
        'C:\Program Files\sccache\sccache.exe',
        'C:\Program Files (x86)\sccache\sccache.exe'
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

if (-not $ErrorsOnly) {
    Write-Host "vcvars: $vcvars"
    Write-Host "cmake : $cmake"
    Write-Host "ninja : $ninja"
    if ($sccache) {
        Write-Host "sccache: $sccache"
    }
}

if ($CleanDeps) {
    if (Test-Path -LiteralPath $buildDir) {
        Remove-Item -Recurse -Force -LiteralPath $buildDir
    }
    $fetchcontentDir = Join-Path $repoRoot '.cache\fetchcontent'
    if (Test-Path -LiteralPath $fetchcontentDir) {
        Remove-Item -Recurse -Force -LiteralPath $fetchcontentDir
    }
} elseif ($Clean -and (Test-Path -LiteralPath $buildDir)) {
    Remove-Item -Recurse -Force -LiteralPath $buildDir
}

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

$configureNeeded = $Reconfigure -or -not (Test-Path -LiteralPath (Join-Path $buildDir 'CMakeCache.txt'))
if ($configureNeeded) {
    $ninjaArg = ($ninja -replace '\\', '/')
    $cmakeArgs = @(
        '--preset', $buildType,
        '-S', $repoRoot,
        "-DCMAKE_MAKE_PROGRAM=$ninjaArg"
    )

    if ($sccache) {
        $sccacheArg = ($sccache -replace '\\', '/')
        $cmakeArgs += "-DCMAKE_CXX_COMPILER_LAUNCHER=$sccacheArg"
        $cmakeArgs += "-DCMAKE_C_COMPILER_LAUNCHER=$sccacheArg"
    }

    Invoke-WithVcvars -Vcvars $vcvars -Executable $cmake -Arguments $cmakeArgs
}

if ($sccache) {
    & $sccache --zero-stats | Out-Null
}

if ($CoreOnly) {
    $buildArgs = @('--build', '--preset', $buildType, '--target', 'TachyonCore')
} elseif ($TestsOnly) {
    $buildArgs = @('--build', '--preset', $buildType, '--target', 'TachyonTests')
} else {
    $buildArgs = @('--build', '--preset', $buildType)
}

if ($ErrorsOnly) {
    $output = Invoke-WithVcvars -Vcvars $vcvars -Executable $cmake -Arguments $buildArgs -PassThru
    $output | Select-String -Pattern "error C|fatal error|FAILED|Error:"
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $($LASTEXITCODE): $cmake"
    }
} else {
    Invoke-WithVcvars -Vcvars $vcvars -Executable $cmake -Arguments $buildArgs
}

if ($ShowSccacheStats -and $sccache) {
    & $sccache --show-stats
}

if ($Test) {
    $testExe = Join-Path $buildDir 'tests\TachyonTests.exe'
    if (-not (Test-Path $testExe)) {
        Write-Host "Test executable not found. Building TachyonTests..."
        $testArgs = @('--preset', $buildType, '--target', 'TachyonTests')
        Invoke-WithVcvars -Vcvars $vcvars -Executable $cmake -Arguments $testArgs
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
