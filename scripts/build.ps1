param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$ConfigurePreset = "OCCTDebug-Debug",
    [string]$BuildPreset = "OCCTDebug-Debug",
    [string]$TestDir = "",
    [string]$TestRegex = "",
    [switch]$NoConfigure,
    [switch]$NoBuild,
    [switch]$NoTest,
    [switch]$CleanFirst
)

$ErrorActionPreference = "Stop"

function Find-FirstExistingPath {
    param([string[]]$Candidates)

    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate)) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    return ""
}

function Find-VisualStudioDevCmd {
    if ($env:OCCTDEBUG_VSDEVCMD -and (Test-Path -LiteralPath $env:OCCTDEBUG_VSDEVCMD)) {
        return (Resolve-Path -LiteralPath $env:OCCTDEBUG_VSDEVCMD).Path
    }

    $vswhereCandidates = @(
        (Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"),
        (Join-Path $env:ProgramFiles "Microsoft Visual Studio\Installer\vswhere.exe")
    )
    $vswhere = Find-FirstExistingPath $vswhereCandidates

    if ($vswhere) {
        try {
            $installationPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null |
                Select-Object -First 1
            if ($installationPath) {
                $candidate = Join-Path ([string]$installationPath) "Common7\Tools\VsDevCmd.bat"
                if (Test-Path -LiteralPath $candidate) {
                    return (Resolve-Path -LiteralPath $candidate).Path
                }
            }
        } catch {
        }
    }

    if ($env:VSINSTALLDIR) {
        $candidate = Join-Path $env:VSINSTALLDIR "Common7\Tools\VsDevCmd.bat"
        if (Test-Path -LiteralPath $candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    return ""
}

function Invoke-BuildCommand {
    param(
        [string]$Command,
        [string]$DevCmd
    )

    Push-Location $RepoRoot
    try {
        if ($DevCmd) {
            $cmdLine = "`"$DevCmd`" -arch=x64 -host_arch=x64 >nul && $Command"
            & cmd.exe /d /s /c $cmdLine
        } else {
            Write-Host "Visual Studio developer environment was not found; running command with current PATH."
            & cmd.exe /d /s /c $Command
        }

        if ($LASTEXITCODE -ne 0) {
            throw "Command failed with exit code ${LASTEXITCODE}: $Command"
        }
    } finally {
        Pop-Location
    }
}

$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
$devCmd = Find-VisualStudioDevCmd

if (-not $TestDir) {
    $TestDir = Join-Path $RepoRoot "out/build/debug"
}

if ($CleanFirst) {
    Invoke-BuildCommand -DevCmd $devCmd -Command "cmake --build --preset `"$BuildPreset`" --clean-first"
} else {
    if (-not $NoConfigure) {
        Invoke-BuildCommand -DevCmd $devCmd -Command "cmake --preset `"$ConfigurePreset`""
    }

    if (-not $NoBuild) {
        Invoke-BuildCommand -DevCmd $devCmd -Command "cmake --build --preset `"$BuildPreset`""
    }
}

if (-not $NoTest) {
    $resolvedTestDir = if (Test-Path -LiteralPath $TestDir) {
        (Resolve-Path -LiteralPath $TestDir).Path
    } else {
        $TestDir
    }

    $ctestCommand = "ctest --test-dir `"$resolvedTestDir`" --output-on-failure"
    if ($TestRegex) {
        $ctestCommand += " -R `"$TestRegex`""
    }
    Invoke-BuildCommand -DevCmd $devCmd -Command $ctestCommand
}

Write-Host "OCCTDEBUG_BUILD_OK"
