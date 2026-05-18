param(
    [Parameter(Mandatory=$true)]
    [string]$DllPath
)

if (!(Test-Path $DllPath)) {
    Write-Error "DLL not found: $DllPath"
    exit 2
}

$required = @(
    "tachyon_version",
    "tachyon_init",
    "tachyon_run",
    "tachyon_init_render_options",
    "tachyon_render"
)

$dumpbin = Get-Command dumpbin -ErrorAction SilentlyContinue
if (-not $dumpbin) {
    Write-Error "dumpbin not found. Run inside MSVC Developer Command Prompt."
    exit 2
}

$exports = & dumpbin /EXPORTS $DllPath | Out-String

foreach ($sym in $required) {
    if ($exports -notmatch "\b$sym\b") {
        Write-Error "Missing exported symbol: $sym"
        exit 1
    }
}

Write-Host "All required C API symbols are exported."
