param(
[string]$SceneDir = "tests\legacy\visual\3d\scenes",
[string]$OutputDir = "tests\visual\3d\actual",
[string]$TachyonExe = ".\build\src\RelWithDebInfo\tachyon.exe",
[switch]$Sharp
)

if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}

function Set-JsonProperty {
    param(
        [Parameter(Mandatory=$true)] $Object,
        [Parameter(Mandatory=$true)] [string] $Name,
        [Parameter(Mandatory=$true)] $Value
    )

    if ($null -eq $Object.PSObject.Properties[$Name]) {
        $Object | Add-Member -MemberType NoteProperty -Name $Name -Value $Value -Force
    } else {
        $Object.$Name = $Value
    }
}

$scenes = Get-ChildItem -Path $SceneDir -Filter "test_*.json"

foreach ($scene in $scenes) {
    $testId = $scene.BaseName -replace "test_", ""
    $jobFile = Join-Path $SceneDir "job_$testId.json"
    
    if (-not (Test-Path $jobFile)) {
        Write-Warning "Job file not found for $($scene.Name), skipping."
        continue
    }

    $outPng = Join-Path $OutputDir "$($scene.BaseName).png"
    $job = Get-Content $jobFile -Raw | ConvertFrom-Json
    $frame = 0
    if ($null -ne $job.preview_frame) {
        $frame = [int]$job.preview_frame
    } elseif ($null -ne $job.frame) {
        $frame = [int]$job.frame
    }

    $effectiveJobFile = $jobFile
    $tempJobFile = $null
    if ($Sharp) {
        Set-JsonProperty -Object $job -Name "quality_tier" -Value "inspect"
        Set-JsonProperty -Object $job -Name "motion_blur_enabled" -Value $false
        Set-JsonProperty -Object $job -Name "motion_blur_samples" -Value 1
        Set-JsonProperty -Object $job -Name "motion_blur_shutter_angle" -Value 0
        $tempJobFile = Join-Path $env:TEMP "$($scene.BaseName).sharp.job.json"
        $job | ConvertTo-Json -Depth 32 | Set-Content -Encoding UTF8 $tempJobFile
        $effectiveJobFile = $tempJobFile
    }

    Write-Host "Rendering $($scene.Name)..." -ForegroundColor Cyan
    
    & $TachyonExe preview-frame --scene $scene.FullName --job $effectiveJobFile --frame $frame --out $outPng
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  [SUCCESS] Saved to $outPng" -ForegroundColor Green
    } else {
        Write-Error "  [FAILED] Failed to render $($scene.Name)"
    }

    if ($tempJobFile -and (Test-Path $tempJobFile)) {
        Remove-Item $tempJobFile -Force
    }
}
