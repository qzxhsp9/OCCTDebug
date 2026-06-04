param(
    [Parameter(Mandatory = $true)]
    [string]$TclScript,
    [string]$DrawLogDir = "",
    [string]$RepoRoot = "",
    [string]$CaseId = "DRAW-REPRO",
    [string]$OutputDir = ""
)

$ErrorActionPreference = "Stop"

function Get-ScriptRoot {
    if ($PSScriptRoot) {
        return $PSScriptRoot
    }
    return (Split-Path -Parent $MyInvocation.MyCommand.Path)
}

if (-not (Test-Path -LiteralPath $TclScript)) {
    throw "Tcl script does not exist: $TclScript"
}

if (-not $RepoRoot) {
    $RepoRoot = (Resolve-Path (Join-Path (Get-ScriptRoot) "..")).Path
}
$repoRootPath = (Resolve-Path -LiteralPath $RepoRoot).Path
if (-not $OutputDir) {
    $OutputDir = Join-Path $repoRootPath "out/repro_packs/$CaseId"
}

$packRoot = $OutputDir
New-Item -ItemType Directory -Path $packRoot -Force | Out-Null
$scriptsDir = Join-Path $packRoot "scripts"
$logsDir = Join-Path $packRoot "logs"
New-Item -ItemType Directory -Path $scriptsDir -Force | Out-Null
New-Item -ItemType Directory -Path $logsDir -Force | Out-Null

$tclSource = (Resolve-Path -LiteralPath $TclScript).Path
Copy-Item -LiteralPath $tclSource -Destination (Join-Path $scriptsDir "repro.tcl") -Force

$copiedLogs = @()
if ($DrawLogDir -and (Test-Path -LiteralPath $DrawLogDir)) {
    Get-ChildItem -LiteralPath $DrawLogDir -File -ErrorAction SilentlyContinue | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $logsDir -Force
        $copiedLogs += $_.Name
    }
}

$manifest = [ordered]@{
    schema_version = 1
    case_id = $CaseId
    generated_at = (Get-Date).ToString("o")
    repro_type = "DRAW"
    script = "scripts/repro.tcl"
    logs = @($copiedLogs)
    note = "Minimal local Repro Pack. No CAD data is included unless explicitly copied by the caller."
}
$manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $packRoot "manifest.json") -Encoding UTF8

[ordered]@{
    repro_pack = $packRoot
    manifest = Join-Path $packRoot "manifest.json"
} | ConvertTo-Json -Depth 4
