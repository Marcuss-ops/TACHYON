param(
    [switch]$ReportOnly,
    [switch]$IncludeSources
)

$ErrorActionPreference = 'Stop'

$repoRoot = (Get-Location).Path

function Write-Section([string]$title) {
    Write-Host ""
    Write-Host "=== $title ==="
}

function Get-Text([string]$path) {
    if (-not (Test-Path -LiteralPath $path)) {
        return ""
    }
    return Get-Content -LiteralPath $path -Raw
}

function Get-RootMarkdownOutliers {
    $allowed = @(
        'README.md',
        'AGENTS.md',
        'CONTRIBUTING.md',
        'TACHYON_ENGINEERING_RULES.md',
        'LICENSE',
        'LICENSE.md'
    )

    Get-ChildItem -LiteralPath $repoRoot -File -Filter *.md | ForEach-Object {
        if ($allowed -notcontains $_.Name) {
            $_.Name
        }
    }
}

function Get-LayerSpecAuditFields {
    $docPath = Join-Path $repoRoot 'docs/layerspec-domain-ownership.md'
    $fields = New-Object System.Collections.Generic.HashSet[string]
    if (-not (Test-Path -LiteralPath $docPath)) {
        return $fields
    }

    foreach ($line in Get-Content -LiteralPath $docPath) {
        if ($line -match '^\|\s*`([^`]+)`\s*\|') {
            [void]$fields.Add($Matches[1])
        }
    }

    return $fields
}

function Get-LayerSpecFields {
    $header = Join-Path $repoRoot 'include/tachyon/core/spec/schema/objects/layer_spec.h'
    $fields = New-Object System.Collections.Generic.List[string]
    if (-not (Test-Path -LiteralPath $header)) {
        return $fields
    }

    $inStruct = $false
    $depth = 0
    foreach ($line in Get-Content -LiteralPath $header) {
        if ($line -match '^\s*struct\s+LayerSpec\s*\{') {
            $inStruct = $true
            $depth = 1
            continue
        }
        if (-not $inStruct) {
            continue
        }
        if ($depth -eq 1 -and $line -match '^\s*(?:.+?)\s+([A-Za-z_][A-Za-z0-9_]*)\s*(?:\{|=|;)\s*(?:\/\/.*)?$') {
            $name = $Matches[1]
            if ($name -notin @('LayerSpec', 'MarkerSpec')) {
                $fields.Add($name)
            }
        }
        $open = ([regex]::Matches($line, '\{')).Count
        $close = ([regex]::Matches($line, '\}')).Count
        $depth += $open - $close
        if ($inStruct -and $depth -le 0) {
            break
        }
    }

    return $fields
}

function Get-SourceListText {
    $paths = @(
        'cmake/tachyon/TachyonCoreSources.cmake',
        'cmake/tachyon/TachyonSceneSources.cmake',
        'cmake/tachyon/TachyonCLISources.cmake',
        'tests/CMakeLists.txt'
    )

    $text = ""
    foreach ($rel in $paths) {
        $path = Join-Path $repoRoot $rel
        if (Test-Path -LiteralPath $path) {
            $text += "`n" + (Get-Content -LiteralPath $path -Raw)
        }
    }
    return $text
}

function Test-SourceReferenced([string]$relativePath, [string]$sourceText) {
    $normalized = $relativePath.Replace('\', '/')
    $simple = $normalized
    if ($normalized.StartsWith('src/')) {
        $simple = $normalized.Substring(4)
    }
    return ($sourceText -match [regex]::Escape($normalized)) -or ($sourceText -match [regex]::Escape($simple))
}

$issues = New-Object System.Collections.Generic.List[string]

Write-Section "Root markdown outliers"
$rootOutliers = Get-RootMarkdownOutliers
if ($rootOutliers.Count -eq 0) {
    Write-Host "  OK"
} else {
    foreach ($item in $rootOutliers) {
        $msg = "  [root-md] $item is not an approved root markdown file"
        Write-Host $msg
        $issues.Add($msg) | Out-Null
    }
}

Write-Section "Typo / dead artifacts"
$typoPatterns = @(
    'txtnetxtimplmementation\.txt',
    'backgorund',
    '^Possible Optimizations\.md$',
    '^MVP_V1\.md$'
)
$typoHits = Get-ChildItem -LiteralPath $repoRoot -Recurse -File | Where-Object {
    $name = $_.Name
    foreach ($pattern in $typoPatterns) {
        if ($name -match $pattern -or $_.FullName -match $pattern) {
            return $true
        }
    }
    return $false
}
if ($typoHits.Count -eq 0) {
    Write-Host "  OK"
} else {
    foreach ($item in $typoHits) {
        $msg = "  [typo] $($item.FullName.Substring($repoRoot.Length + 1))"
        Write-Host $msg
        $issues.Add($msg) | Out-Null
    }
}

Write-Section "LayerSpec audit"
$auditFields = Get-LayerSpecAuditFields
$layerFields = Get-LayerSpecFields
$coreAllowlist = @(
    'id','name','type','type_string','asset_id','preset_id','blend_mode','enabled','visible','is_3d',
    'is_adjustment_layer','motion_blur','start_time','in_point','out_point','opacity','width','height',
    'transform','transform3d','opacity_property','mask_feather','time_remap_property','parent',
    'track_matte_layer_id','track_matte_type','precomp_id','duration','loop','hold_last_frame'
)
$tracked = New-Object System.Collections.Generic.List[string]
foreach ($field in $layerFields) {
    if ($coreAllowlist -contains $field) {
        continue
    }
    if ($auditFields.Contains($field)) {
        continue
    }
    $tracked.Add($field) | Out-Null
}

if ($tracked.Count -eq 0) {
    Write-Host "  OK"
} else {
    foreach ($field in $tracked) {
        $msg = "  [layerspec] untracked field: $field"
        Write-Host $msg
        $issues.Add($msg) | Out-Null
    }
}

if ($IncludeSources) {
    Write-Section "Source coverage"
    $sourceText = Get-SourceListText
    $unreferencedSources = New-Object System.Collections.Generic.List[string]
    Get-ChildItem -LiteralPath (Join-Path $repoRoot 'src') -Recurse -File -Filter *.cpp | ForEach-Object {
        $relative = $_.FullName.Substring($repoRoot.Length + 1).Replace('\', '/')
        if (-not (Test-SourceReferenced $relative $sourceText)) {
            $unreferencedSources.Add($relative) | Out-Null
        }
    }

    if ($unreferencedSources.Count -eq 0) {
        Write-Host "  OK"
    } else {
        foreach ($source in $unreferencedSources) {
            $msg = "  [source] not referenced by CMake lists: $source"
            Write-Host $msg
            $issues.Add($msg) | Out-Null
        }
    }
}

Write-Host ""
if ($issues.Count -eq 0) {
    Write-Host "Repo audit passed."
    exit 0
}

Write-Host "Repo audit found $($issues.Count) issue(s)."
if ($ReportOnly) {
    Write-Host "Report-only mode: no files were changed."
}
exit 1
