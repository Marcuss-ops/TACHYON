param()

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-TestRoutingRules {
    $routingPath = Join-Path $PSScriptRoot "test-routing.json"
    if (-not (Test-Path $routingPath)) {
        throw "Missing test routing table: $routingPath"
    }

    return Get-Content -Raw -Path $routingPath | ConvertFrom-Json
}

function Get-TestTargetForFilter {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Filter
    )

    $normalized = ($Filter.ToLowerInvariant() -replace '[^a-z0-9]', '')
    foreach ($rule in (Get-TestRoutingRules)) {
        foreach ($pattern in @($rule.patterns)) {
            if ($pattern -and ($normalized -match $pattern)) {
                return [string]$rule.target
            }
        }
    }

    return "TachyonTests"
}
