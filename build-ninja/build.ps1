# Build Tachyon with proper Windows SDK paths
# This script sets up the Visual Studio environment and builds with Ninja

# Set up Visual Studio environment
$vsPath = "C:\Program Files\Microsoft Visual Studio\2026\Community"
if (Test-Path $vsPath) {
    Write-Host "Setting up Visual Studio 2026 environment..."
    cmd /c "call `"$vsPath\VC\Auxiliary\Build\vcvars64.bat`" && set" | ForEach-Object {
        if ($_ -match "^(.*?)=(.*)$") {
            [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2])
        }
    }
} else {
    Write-Host "Visual Studio 2026 not found, trying 2022..."
    $vsPath = "C:\Program Files\Microsoft Visual Studio\2022\Community"
    if (Test-Path $vsPath) {
        cmd /c "call `"$vsPath\VC\Auxiliary\Build\vcvars64.bat`" && set" | ForEach-Object {
            if ($_ -match "^(.*?)=(.*)$") {
                [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2])
            }
        }
    }
}

# Add Windows SDK include paths
$windowsKits = "C:\Program Files (x86)\Windows Kits\10\Include"
if (Test-Path $windowsKits) {
    $sdkVersions = Get-ChildItem $windowsKits | Where-Object { $_.PSIsContainer } | Sort-Object Name -Descending
    if ($sdkVersions) {
        $latestSdk = $sdkVersions[0].Name
        $sdkInclude = "$windowsKits\$latestSdk\ucrt"
        if (Test-Path $sdkInclude) {
            $env:INCLUDE += ";$sdkInclude"
            Write-Host "Added Windows SDK include path: $sdkInclude"
        }
    }
}

# Change to Tachyon build directory
Set-Location "C:\Users\pater\Pyt\Tachyon\build-ninja"

# Run Ninja build
Write-Host "Starting Ninja build..."
ninja TachyonCore

Write-Host "Build complete!"
