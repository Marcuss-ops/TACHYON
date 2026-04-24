param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
)

$libraryRoot = Join-Path $Root 'studio\library'
$manifestPath = Join-Path $libraryRoot 'system\manifest.json'

function Read-JsonFile {
    param([string]$Path)
    if (-not (Test-Path $Path)) {
        throw "Missing file: $Path"
    }
    Get-Content -Path $Path -Raw | ConvertFrom-Json
}

$manifest = [ordered]@{
    schema = 'tachyon.library_manifest/2.0'
    generated_by = 'tachyon-studio'
    schemas = [ordered]@{
        scene = 'system/scene.schema.json'
    }
    content = [ordered]@{
        lexicon = [ordered]@{
            phrases = 'content/lexicon/phrases'
            words = 'content/lexicon/words'
            names = 'content/lexicon/names'
            numbers = 'content/lexicon/numbers'
        }
        images = 'content/images'
    }
    animations = [ordered]@{
        transitions = @()
        text = @()
        stock = @(
            [ordered]@{ id = 'stock'; path = 'animations/stock' }
        )
    }
    scenes = @()
    system = [ordered]@{
        stack = 'system/stack'
    }
}

$scenesDir = Join-Path $libraryRoot 'scenes'
if (Test-Path $scenesDir) {
    Get-ChildItem -Path $scenesDir -Filter *.json | Sort-Object Name | ForEach-Object {
        $scene = Read-JsonFile $_.FullName
        $manifest.scenes += [ordered]@{
            id = $scene.project.id
            name = $scene.project.name
            path = ('scenes/{0}' -f $_.Name)
        }
    }
}

$transitionsDir = Join-Path $libraryRoot 'animations\transitions'
if (Test-Path $transitionsDir) {
    Get-ChildItem -Path $transitionsDir -Directory | Sort-Object Name | ForEach-Object {
        $metaPath = Join-Path $_.FullName 'meta.json'
        if (Test-Path $metaPath) {
            $meta = Read-JsonFile $metaPath
            $outputDir = Join-Path $_.FullName 'output'
            if (-not (Test-Path $outputDir)) {
                New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
            }
            $demoPath = Join-Path $_.FullName 'demo.json'
            $demo = [ordered]@{
                schema = 'tachyon.transition_demo/1.0'
                transition_id = $meta.id
                name = ("{0} Demo" -f $meta.name)
                source_scene = '../../../scenes/scene_a_white.json'
                target_scene = '../../../scenes/scene_b_red.json'
                transition = [ordered]@{
                    meta = 'meta.json'
                    shader = ([string]$meta.entry)
                    duration_seconds = [double]$meta.duration_seconds
                    progress = [ordered]@{
                        start = 0.0
                        end = 1.0
                    }
                }
                preview = [ordered]@{
                    frame_count = 12
                    resolution_scale = 0.25
                }
                output = [ordered]@{
                    directory = 'output'
                    file_prefix = $meta.id
                    format = 'mp4'
                }
            } | ConvertTo-Json -Depth 8
            Set-Content -Path $demoPath -Value $demo -Encoding utf8
            if (-not (Test-Path (Join-Path $outputDir '.gitkeep'))) {
                Set-Content -Path (Join-Path $outputDir '.gitkeep') -Value '' -NoNewline
            }
            $manifest.animations.transitions += [ordered]@{
                id = $meta.id
                name = $meta.name
                path = ('animations/transitions/{0}/meta.json' -f $_.Name)
                duration_seconds = [double]$meta.duration_seconds
                demo_path = ('animations/transitions/{0}/demo.json' -f $_.Name)
                output_path = ('animations/transitions/{0}/output' -f $_.Name)
            }
        }
    }
}

$textDir = Join-Path $libraryRoot 'animations\typography'
if (Test-Path $textDir) {
    Get-ChildItem -Path $textDir -Directory | Sort-Object Name | ForEach-Object {
        $metaPath = Join-Path $_.FullName 'meta.json'
        if (Test-Path $metaPath) {
            $meta = Read-JsonFile $metaPath
            $manifest.animations.text += [ordered]@{
                id = $meta.id
                name = $meta.name
                path = ('animations/typography/{0}/meta.json' -f $_.Name)
                duration_seconds = [double]$meta.duration_seconds
            }
        }
    }
}

$manifest | ConvertTo-Json -Depth 8 | Set-Content -Path $manifestPath -Encoding utf8
Write-Host "Wrote $manifestPath"
