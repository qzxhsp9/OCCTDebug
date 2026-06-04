param(
    [Parameter(Mandatory = $true)]
    [string]$LogPath,
    [string]$OutputPath = ""
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $LogPath)) {
    throw "DRAW log does not exist: $LogPath"
}

$logPathResolved = (Resolve-Path -LiteralPath $LogPath).Path
$text = Get-Content -LiteralPath $logPathResolved -Raw
$lines = @($text -split "`r?`n")

$errorPatterns = 'Exception|Faulty|invalid command|DRAW_SMOKE_FAILED|DRAW_SMOKE_ERROR|(^|\s)Error(:|\s|$)'
$errorLines = @($lines | Where-Object { $_ -match $errorPatterns })
$tokens = @($lines | Where-Object { $_ -match 'DRAW_[A-Z0-9_]+_OK|DRAW_SMOKE_OK' })
$checkshapeLines = @($lines | Where-Object { $_ -match 'checkshape|This shape seems to be valid|Faulty|valid' })

$checkshapeStatus = "not_detected"
if ($checkshapeLines.Count -gt 0) {
    $checkshapeStatus = "unknown"
}
if ($checkshapeLines -match 'This shape seems to be valid') {
    $checkshapeStatus = "valid"
}
if ($checkshapeLines -match 'Faulty') {
    $checkshapeStatus = "faulty"
}

$result = [ordered]@{
    schema_version = 1
    log = $logPathResolved
    line_count = $lines.Count
    success_tokens = @($tokens)
    error_count = $errorLines.Count
    error_lines = @($errorLines)
    checkshape = [ordered]@{
        status = $checkshapeStatus
        lines = @($checkshapeLines)
    }
}

$json = $result | ConvertTo-Json -Depth 8
if ($OutputPath) {
    $parent = Split-Path -Parent $OutputPath
    if ($parent -and -not (Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Path $parent | Out-Null
    }
    Set-Content -LiteralPath $OutputPath -Value $json -Encoding UTF8
}

$json
