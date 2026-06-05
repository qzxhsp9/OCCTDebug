param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$BuildDir = "",
    [string]$Executable = "",
    [switch]$BuildIfMissing,
    [switch]$Wait
)

$ErrorActionPreference = "Stop"

function Resolve-WorkbenchExecutable {
    param(
        [string]$Root,
        [string]$RequestedBuildDir,
        [string]$ExplicitExecutable
    )

    $candidates = @()
    if ($ExplicitExecutable) {
        $candidates += $ExplicitExecutable
    }

    $buildRoot = if ($RequestedBuildDir) {
        $RequestedBuildDir
    } else {
        Join-Path $Root "out/build/debug"
    }

    $candidates += @(
        (Join-Path $buildRoot "src/OCCTDebug.exe"),
        (Join-Path $buildRoot "src/Debug/OCCTDebug.exe"),
        (Join-Path $buildRoot "OCCTDebug.exe"),
        (Join-Path $buildRoot "Debug/OCCTDebug.exe")
    )

    foreach ($candidate in $candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate)) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    return ""
}

$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path

$exe = Resolve-WorkbenchExecutable -Root $RepoRoot -RequestedBuildDir $BuildDir -ExplicitExecutable $Executable
if (-not $exe -and $BuildIfMissing) {
    & (Join-Path $PSScriptRoot "build.ps1") -RepoRoot $RepoRoot -NoTest
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed before running OCCTDebug."
    }
    $exe = Resolve-WorkbenchExecutable -Root $RepoRoot -RequestedBuildDir $BuildDir -ExplicitExecutable $Executable
}

if (-not $exe) {
    throw "OCCTDebug.exe was not found. Build the project first or pass -BuildIfMissing."
}

$workingDirectory = Split-Path -Parent $exe
if ($Wait) {
    $process = Start-Process -FilePath $exe -WorkingDirectory $workingDirectory -PassThru
    $process.WaitForExit()
    exit $process.ExitCode
}

Start-Process -FilePath $exe -WorkingDirectory $workingDirectory | Out-Null
Write-Host "OCCTDEBUG_RUN_STARTED"
