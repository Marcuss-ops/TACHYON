# Tachyon Build Script (Improved for Windows Developers)
[CmdletBinding()]
param(
    [switch]$Check,
    [switch]$Clean,
    [switch]$Test,
    [switch]$Details,
    [switch]$ErrorsOnly,
    [switch]$CoreOnly,
    [string]$BuildType = "Release"
)

$ErrorActionPreference = "Stop"

# --- Configuration ---
$BuildDir = "build-ninja"
$Generator = "Ninja"
$CMakePath = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$NinjaPath = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
$SccachePath = "sccache"

# --- Helper Functions ---
function Write-Header($msg) {
    Write-Host "`n" + ("=" * 60) -ForegroundColor DarkCyan
    Write-Host "  $msg" -ForegroundColor Cyan
    Write-Host ("=" * 60) -ForegroundColor DarkCyan
}

function Write-Success($msg) {
    Write-Host "  [OK] $msg" -ForegroundColor Green
}

function Write-Failure($msg) {
    Write-Host "  [FAIL] $msg" -ForegroundColor Red
}

function Write-Warning($msg) {
    Write-Host "  [WARN] $msg" -ForegroundColor Yellow
}

function Write-Info($msg) {
    if (-not $ErrorsOnly) {
        Write-Host "  [INFO] $msg" -ForegroundColor Gray
    }
}

function Write-VerboseInfo($msg) {
    if ($Details -and -not $ErrorsOnly) {
        Write-Host "  [DEBUG] $msg" -ForegroundColor DarkGray
    }
}

function Test-CommandExists($cmd) {
    $result = Get-Command $cmd -ErrorAction SilentlyContinue
    return $null -ne $result
}

function Run-Command($cmd, $cmd_args) {
    Write-VerboseInfo "Executing: $cmd $cmd_args"
    
    try {
        & $cmd @cmd_args
        $exitCode = $LASTEXITCODE
        
        if ($exitCode -ne 0) {
            Write-Failure "Command failed with exit code $exitCode"
            throw "Command failed: ${cmd} ${cmd_args}"
        }
    }
    catch {
        Write-Failure "Exception: $_"
        throw
    }
}

# --- Main Build Logic ---
try {
    Write-Header "Tachyon Build Script"
    
    # 1. Environment Setup
    Write-Header "Environment Setup"
    
    # Check if VS environment script exists
    $vsEnvScript = ".\scripts\enable-vs-env.ps1"
    if (Test-Path $vsEnvScript) {
        Write-Info "Loading Visual Studio environment..."
        . $vsEnvScript
    } else {
        Write-Warning "VS environment script not found at $vsEnvScript"
        Write-Info "Attempting to build without VS environment..."
    }
    
    # Verify tools exist
    Write-Info "Verifying tools..."
    $toolsOk = $true
    
    if (Test-CommandExists $CMakePath) {
        Write-Success "CMake found: $CMakePath"
    } else {
        Write-Failure "CMake NOT found at: $CMakePath"
        Write-Info "Please install Visual Studio Build Tools with CMake support"
        $toolsOk = $false
    }
    
    if (Test-CommandExists $NinjaPath) {
        Write-Success "Ninja found: $NinjaPath"
    } else {
        Write-Warning "Ninja NOT found at: $NinjaPath (will rely on CMake's Ninja)"
    }
    
    if (-not $toolsOk) {
        throw "Required tools are missing. Please check the installation."
    }
    
    # Optional: sccache stats reset
    if (Test-CommandExists $SccachePath) {
        Write-Info "Resetting sccache statistics..."
        & $SccachePath --zero-stats 2>&1 | Out-Null
    }
    
    # 2. CMake Configuration (if needed)
    if ($Clean -or -not (Test-Path "$BuildDir\build.ninja")) {
        Write-Header "CMake Configuration"
        
        # Ensure build directory exists
        if (-not (Test-Path $BuildDir)) {
            Write-Info "Creating build directory: $BuildDir"
            New-Item -ItemType Directory -Path $BuildDir | Out-Null
        }
        
        $cmakeArgs = @(
            "-G", $Generator,
            "-B", $BuildDir,
            "-DCMAKE_BUILD_TYPE=$BuildType",
            "-DCMAKE_MAKE_PROGRAM=$NinjaPath"
        )
        
        Run-Command $CMakePath $cmakeArgs
        Write-Success "CMake configuration completed"
    } else {
        Write-Info "Build directory exists, skipping CMake configuration (use -Clean to reconfigure)"
    }
    
    # 3. Build Execution
    Write-Header "Build Execution"
    
    if ($Check) {
        Write-Info "Running syntax check (build.ps1 -Check)"
        $buildArgs = @("--build", $BuildDir, "--config", $BuildType, "--target", "HeaderSmokeTests")
        Run-Command $CMakePath $buildArgs
        Write-Success "Syntax check passed!"
    } elseif ($Test) {
        Write-Info "Building and running tests..."
        $buildArgs = @("--build", $BuildDir, "--config", $BuildType, "--target", "TachyonTests")
        Run-Command $CMakePath $buildArgs
        
        # Copy TachyonCore.dll to the tests directory so it can be found at runtime
        $dllSource = "$BuildDir\src\TachyonCore.dll"
        $dllDest = "$BuildDir\tests\"
        if (Test-Path $dllSource) {
            Write-Info "Copying TachyonCore.dll to tests directory..."
            Copy-Item $dllSource -Destination $dllDest -Force
            Write-Success "DLL copied successfully"
        } else {
            Write-Warning "TachyonCore.dll not found at: $dllSource"
        }
        
        # Run tests
        $testExe = "$BuildDir\tests\TachyonTests.exe"
        if (Test-Path $testExe) {
            Write-Info "Running tests..."
            & $testExe --gtest_output=text
            if ($LASTEXITCODE -ne 0) {
                throw "Tests failed with exit code $LASTEXITCODE"
            }
            Write-Success "All tests passed!"
        } else {
            Write-Warning "Test executable not found at: $testExe"
        }
    } elseif ($CoreOnly) {
        Write-Info "Building TachyonCore only..."
        $buildArgs = @("--build", $BuildDir, "--config", $BuildType, "--target", "TachyonCore")
        Run-Command $CMakePath $buildArgs
        Write-Success "TachyonCore build completed successfully!"
    } else {
        Write-Info "Building all targets..."
        $buildArgs = @("--build", $BuildDir, "--config", $BuildType)
        Run-Command $CMakePath $buildArgs
        Write-Success "Build completed successfully!"
    }
    
    # 4. Summary
    Write-Header "Build Summary"
    Write-Success "Build type: $BuildType"
    Write-Success "Build directory: $BuildDir"
    
    if (Test-CommandExists $SccachePath) {
        Write-Header "sccache Statistics"
        & $SccachePath --show-stats
    }
}
catch {
    Write-Header "Build FAILED"
    Write-Failure $_.Exception.Message
    Write-Host "`nTroubleshooting tips:" -ForegroundColor Yellow
    Write-Host "  1. Ensure Visual Studio Build Tools are installed" -ForegroundColor Yellow
    Write-Host "  2. Run .\scripts\enable-vs-env.ps1 manually to check VS environment" -ForegroundColor Yellow
    Write-Host "  3. Try -Clean flag to rebuild from scratch" -ForegroundColor Yellow
    Write-Host "  4. Check build-ninja\CMakeFiles\CMakeError.log for details" -ForegroundColor Yellow
    exit 1
}
