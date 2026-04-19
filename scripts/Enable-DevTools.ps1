param(
    [switch]$PersistUserPath
)

$cmakeCandidates = @(
    'C:\Program Files\cmake-4.3.0-windows-x86_64\bin',
    'C:\Program Files\CMake\bin',
    'C:\Program Files (x86)\CMake\bin'
)

$msbuildCandidates = @(
    'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin',
    'C:\Program Files\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin'
)

function Resolve-ExistingPath {
    param(
        [string[]]$Candidates
    )

    foreach ($candidate in $Candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    return $null
}

$cmakeBin = Resolve-ExistingPath -Candidates $cmakeCandidates
$msbuildBin = Resolve-ExistingPath -Candidates $msbuildCandidates

if (-not $cmakeBin) {
    throw "CMake bin directory not found. Update scripts/Enable-DevTools.ps1 with the local install path."
}

if (-not $msbuildBin) {
    throw "MSBuild bin directory not found. Update scripts/Enable-DevTools.ps1 with the local install path."
}

$pathsToAdd = @($cmakeBin, $msbuildBin)

function Add-UniquePathEntries {
    param(
        [string]$CurrentValue,
        [string[]]$Entries
    )

    $segments = @()
    if ($CurrentValue) {
        $segments = $CurrentValue -split ';' | Where-Object { $_ -and $_.Trim() -ne '' }
    }

    foreach ($entry in $Entries) {
        if ($segments -notcontains $entry) {
            $segments += $entry
        }
    }

    return ($segments -join ';')
}

$env:PATH = Add-UniquePathEntries -CurrentValue $env:PATH -Entries $pathsToAdd

if ($PersistUserPath) {
    $userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
    [Environment]::SetEnvironmentVariable('Path', (Add-UniquePathEntries -CurrentValue $userPath -Entries $pathsToAdd), 'User')
}

Write-Host "Enabled dev tools:"
Write-Host "  CMake:  $cmakeBin"
Write-Host "  MSBuild: $msbuildBin"
Write-Host "Run this shell again or start a new one to pick up persisted user PATH changes."
