param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$BuildDir = (Join-Path $RepoRoot "out/build/debug"),
    [string]$Config = "Debug",
    [string]$TestName = "draw_smoke",
    [string]$TclScript = "",
    [string]$SuccessToken = "DRAW_SMOKE_OK",
    [string]$DrawExe = "",
    [int]$TimeoutSeconds = 30
)

$ErrorActionPreference = "Stop"

function Resolve-OptionalPath {
    param([string]$Path)

    if ($Path -and (Test-Path -LiteralPath $Path)) {
        return (Resolve-Path -LiteralPath $Path).Path
    }
    return ""
}

function Find-DrawExe {
    param(
        [string]$Root,
        [string]$RequestedConfig,
        [string]$ExplicitDrawExe
    )

    $candidates = @()
    if ($ExplicitDrawExe) {
        $candidates += $ExplicitDrawExe
    }
    if ($env:OCCTDEBUG_DRAWEXE) {
        $candidates += $env:OCCTDEBUG_DRAWEXE
    }

    $normalizedConfig = if ($RequestedConfig) { $RequestedConfig } else { "Debug" }
    $candidates += @(
        (Join-Path $Root "depends/occt/lib/$normalizedConfig/bind/DRAWEXE.exe"),
        (Join-Path $Root "depends/occt/lib/$normalizedConfig/bin/DRAWEXE.exe"),
        (Join-Path $Root "depends/occt/lib/$normalizedConfig/bini/DRAWEXE.exe"),
        (Join-Path $Root "depends/occt/lib/Debug/bind/DRAWEXE.exe"),
        (Join-Path $Root "depends/occt/lib/Release/bin/DRAWEXE.exe"),
        (Join-Path $Root "depends/occt/lib/RelWithDebInfo/bini/DRAWEXE.exe")
    )

    foreach ($candidate in $candidates) {
        $resolved = Resolve-OptionalPath $candidate
        if ($resolved) {
            return $resolved
        }
    }

    $occtRoot = Join-Path $Root "depends/occt"
    if (Test-Path -LiteralPath $occtRoot) {
        $found = Get-ChildItem -LiteralPath $occtRoot -Recurse -Filter DRAWEXE.exe -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if ($found) {
            return $found.FullName
        }
    }

    return ""
}

function Add-ExistingPath {
    param(
        [System.Collections.Generic.List[string]]$PathList,
        [string]$Path
    )

    if ($Path -and (Test-Path -LiteralPath $Path)) {
        $PathList.Add((Resolve-Path -LiteralPath $Path).Path)
    }
}

function Get-DrawOutputSummary {
    param(
        [string]$Stdout,
        [string]$Stderr
    )

    $allLines = @()
    if ($Stdout) {
        $allLines += ($Stdout -split "`r?`n")
    }
    if ($Stderr) {
        $allLines += ($Stderr -split "`r?`n")
    }

    $errorPatterns = 'Exception|Faulty|invalid command|DRAW_SMOKE_FAILED|DRAW_SMOKE_ERROR|(^|\s)Error(:|\s|$)'
    $errorLines = @($allLines | Where-Object { $_ -match $errorPatterns })
    $checkshapeLines = @($allLines | Where-Object { $_ -match 'checkshape|This shape seems to be valid|Faulty|valid' })
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

    return [ordered]@{
        error_count = $errorLines.Count
        error_lines = @($errorLines)
        checkshape = [ordered]@{
            status = $checkshapeStatus
            lines = @($checkshapeLines)
        }
    }
}

$repoRootPath = (Resolve-Path -LiteralPath $RepoRoot).Path
$buildDirPath = $BuildDir
if (-not (Test-Path -LiteralPath $buildDirPath)) {
    New-Item -ItemType Directory -Path $buildDirPath | Out-Null
}
$buildDirPath = (Resolve-Path -LiteralPath $buildDirPath).Path

$safeTestName = $TestName -replace '[^A-Za-z0-9_.-]', '_'
$logDir = Join-Path $buildDirPath "Testing/$safeTestName"
New-Item -ItemType Directory -Path $logDir -Force | Out-Null
$logPath = Join-Path $logDir "$safeTestName.log"
$stdoutPath = Join-Path $logDir "$safeTestName.stdout.log"
$stderrPath = Join-Path $logDir "$safeTestName.stderr.log"
$resultJsonPath = Join-Path $logDir "$safeTestName.result.json"

function Write-SmokeLog {
    param([string]$Message)
    [System.IO.File]::AppendAllText(
        $logPath,
        "$Message`r`n",
        [System.Text.UTF8Encoding]::new($false)
    )
}

function Fail-Smoke {
    param(
        [string]$Message,
        [int]$ExitCode = 1
    )

    Write-SmokeLog "DRAW_SMOKE_FAILED: $Message"
    Write-Host "DRAW_SMOKE_FAILED: $Message"
    Write-Host "DRAW_SMOKE_LOG: $logPath"
    exit $ExitCode
}

Set-Content -LiteralPath $logPath -Value "DRAW test started: $(Get-Date -Format o)" -Encoding UTF8
Write-SmokeLog "test_name=$TestName"
Write-SmokeLog "repo_root=$repoRootPath"
Write-SmokeLog "build_dir=$buildDirPath"
Write-SmokeLog "config=$Config"
Write-SmokeLog "success_token=$SuccessToken"

$drawExePath = Find-DrawExe -Root $repoRootPath -RequestedConfig $Config -ExplicitDrawExe $DrawExe
if (-not $drawExePath) {
    Fail-Smoke "DRAWEXE.exe was not found. Set OCCTDEBUG_DRAWEXE or place OCCT under depends/occt." 2
}

$occtRoot = Join-Path $repoRootPath "depends/occt"
$drawResources = Join-Path $occtRoot "src/DrawResources/DrawDefault"
if (-not (Test-Path -LiteralPath $drawResources)) {
    Fail-Smoke "DRAW resources are missing: $drawResources" 3
}

$tclScript = if ($TclScript) { $TclScript } else { Join-Path $repoRootPath "tests/draw_smoke.tcl" }
if (-not (Test-Path -LiteralPath $tclScript)) {
    Fail-Smoke "Smoke Tcl script is missing: $tclScript" 4
}
$tclScript = (Resolve-Path -LiteralPath $tclScript).Path

$tcltkRoot = Join-Path $repoRootPath "depends/occt_3rdparty/tcltk-8.6.15-x64"
$freetypeRoot = Join-Path $repoRootPath "depends/occt_3rdparty/freetype-2.13.3-x64"
$tclLibrary = Join-Path $tcltkRoot "lib/tcl8.6"
$tkLibrary = Join-Path $tcltkRoot "lib/tk8.6"

if (-not (Test-Path -LiteralPath (Join-Path $tcltkRoot "bin/tcl86.dll"))) {
    Fail-Smoke "tcl86.dll was not found under depends/occt_3rdparty/tcltk-8.6.15-x64/bin." 5
}
if (-not (Test-Path -LiteralPath (Join-Path $tcltkRoot "bin/tk86.dll"))) {
    Fail-Smoke "tk86.dll was not found under depends/occt_3rdparty/tcltk-8.6.15-x64/bin." 6
}

$pathEntries = [System.Collections.Generic.List[string]]::new()
Add-ExistingPath $pathEntries (Split-Path -Parent $drawExePath)
Add-ExistingPath $pathEntries (Join-Path $freetypeRoot "bin")
Add-ExistingPath $pathEntries (Join-Path $tcltkRoot "bin")
Add-ExistingPath $pathEntries (Join-Path $tcltkRoot "lib")
$pathEntries.Add($env:PATH)

Write-SmokeLog "drawexe=$drawExePath"
Write-SmokeLog "casroot=$occtRoot"
Write-SmokeLog "tcl_library=$tclLibrary"
Write-SmokeLog "tk_library=$tkLibrary"
Write-SmokeLog "stdout=$stdoutPath"
Write-SmokeLog "stderr=$stderrPath"

$scriptBytes = [System.IO.File]::ReadAllBytes($tclScript)
if ($scriptBytes.Length -ge 3 -and
    $scriptBytes[0] -eq 0xEF -and
    $scriptBytes[1] -eq 0xBB -and
    $scriptBytes[2] -eq 0xBF) {
    $scriptBytes = $scriptBytes[3..($scriptBytes.Length - 1)]
}
$scriptText = [System.Text.Encoding]::UTF8.GetString($scriptBytes)
$runtimeTclPath = Join-Path $logDir "draw_smoke.tcl"
Set-Content -LiteralPath $runtimeTclPath -Value $scriptText -Encoding ASCII
Write-SmokeLog "source_tcl=$tclScript"
Write-SmokeLog "runtime_tcl=$runtimeTclPath"

$processInfo = [System.Diagnostics.ProcessStartInfo]::new()
$processInfo.FileName = if ($env:ComSpec) { $env:ComSpec } else { "cmd.exe" }
$processInfo.Arguments = '/d /s /c ""{0}" < "{1}""' -f $drawExePath, $runtimeTclPath
$processInfo.WorkingDirectory = $logDir
$processInfo.UseShellExecute = $false
$processInfo.RedirectStandardOutput = $true
$processInfo.RedirectStandardError = $true
$processInfo.Environment["CASROOT"] = $occtRoot
$processInfo.Environment["TCL_LIBRARY"] = $tclLibrary
$processInfo.Environment["TK_LIBRARY"] = $tkLibrary
$processInfo.Environment["PATH"] = ($pathEntries -join ";")

$process = [System.Diagnostics.Process]::new()
$process.StartInfo = $processInfo

try {
    if (-not $process.Start()) {
        Fail-Smoke "Failed to start DRAWEXE.exe." 7
    }

    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        try {
            $process.Kill()
        } catch {
        }
        Fail-Smoke "DRAWEXE.exe timed out after $TimeoutSeconds seconds." 8
    }

    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    Set-Content -LiteralPath $stdoutPath -Value $stdout -Encoding UTF8
    Set-Content -LiteralPath $stderrPath -Value $stderr -Encoding UTF8

    Write-Host $stdout
    if ($stderr) {
        Write-Host $stderr
    }

    Write-SmokeLog "exit_code=$($process.ExitCode)"
    if ($process.ExitCode -ne 0) {
        Fail-Smoke "DRAWEXE.exe exited with code $($process.ExitCode)." 9
    }

    $result = [ordered]@{
        schema_version = 1
        test_name = $TestName
        drawexe = $drawExePath
        tcl_script = $tclScript
        runtime_tcl = $runtimeTclPath
        log = $logPath
        stdout = $stdoutPath
        stderr = $stderrPath
        exit_code = $process.ExitCode
        success_token = $SuccessToken
        success_token_found = [bool]((-not $SuccessToken) -or ($stdout -match [regex]::Escape($SuccessToken)))
        output_summary = Get-DrawOutputSummary -Stdout $stdout -Stderr $stderr
        generated_at = (Get-Date).ToString("o")
    }
    $result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $resultJsonPath -Encoding UTF8
    Write-SmokeLog "result_json=$resultJsonPath"

    if ($SuccessToken -and $stdout -notmatch [regex]::Escape($SuccessToken)) {
        Fail-Smoke "$SuccessToken was not found in DRAW stdout." 10
    }

    if ($SuccessToken) {
        Write-SmokeLog $SuccessToken
    }
    Write-Host "DRAW_SMOKE_LOG: $logPath"
    exit 0
} catch {
    Write-SmokeLog "DRAW_SMOKE_ERROR: $($_.Exception.Message)"
    Write-Host "DRAW_SMOKE_ERROR: $($_.Exception.Message)"
    Write-Host "DRAW_SMOKE_LOG: $logPath"
    exit 11
} finally {
    if ($process) {
        $process.Dispose()
    }
}
