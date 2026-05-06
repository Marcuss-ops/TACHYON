param()

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$root = $PSScriptRoot
$repoRoot = Split-Path -Parent $root
$readmePath = Join-Path $repoRoot "tests/disabled/README.md"
$cmakePath = Join-Path $repoRoot "tests/CMakeLists.txt"

if (-not (Test-Path $readmePath)) {
    throw "Missing disabled-tests README: $readmePath"
}

$readme = Get-Content -Raw -Path $readmePath
$cmake = Get-Content -Raw -Path $cmakePath

$entries = [regex]::Matches($readme, '^###\s+([A-Za-z0-9_]+)\s*$', 'Multiline') | ForEach-Object { $_.Groups[1].Value }
if ($entries.Count -eq 0) {
    throw "No disabled-test entries found in tests/disabled/README.md"
}

$documentedFiles = [regex]::Matches($readme, 'File:\s+`([^`]+)`') | ForEach-Object { $_.Groups[1].Value }
foreach ($file in $documentedFiles) {
    $resolved = Join-Path $repoRoot $file
    if (-not (Test-Path $resolved)) {
        throw "Disabled test file listed in README does not exist: $file"
    }
}

$commentedSources = [regex]::Matches($cmake, '^\s*#\s*(unit/[^#"\s]+\.cpp)', 'Multiline') | ForEach-Object { $_.Groups[1].Value }
foreach ($source in $commentedSources) {
    # Ignore comment-only annotations when the same source is also active elsewhere.
    if ($cmake -match ('(?m)^\s*' + [regex]::Escape($source) + '\s*$')) {
        continue
    }

    $sourceName = [IO.Path]::GetFileNameWithoutExtension($source)
    if ($readme -notmatch [regex]::Escape($sourceName)) {
        throw "Commented-out test source is not documented in tests/disabled/README.md: $source"
    }
}

Write-Host "Disabled-test documentation check passed." -ForegroundColor Green
