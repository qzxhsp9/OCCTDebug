param(
    [Parameter(Mandatory = $true)]
    [string]$ReproTcl,
    [string]$RepoRoot = "",
    [string]$BuildDir = "",
    [string]$Config = "Debug",
    [string]$TestName = "draw_temp_repro",
    [string]$SuccessToken = ""
)

$ErrorActionPreference = "Stop"

function Get-ScriptRoot {
    if ($PSScriptRoot) {
        return $PSScriptRoot
    }
    return (Split-Path -Parent $MyInvocation.MyCommand.Path)
}

function ConvertTo-CMakePath {
    param([string]$Path)
    return ((Resolve-Path -LiteralPath $Path).Path -replace '\\', '/')
}

if (-not (Test-Path -LiteralPath $ReproTcl)) {
    throw "repro.tcl does not exist: $ReproTcl"
}

if (-not $RepoRoot) {
    $RepoRoot = (Resolve-Path (Join-Path (Get-ScriptRoot) "..")).Path
}
$repoRootPath = (Resolve-Path -LiteralPath $RepoRoot).Path
if (-not $BuildDir) {
    $BuildDir = Join-Path $repoRootPath "out/build/debug"
}
if (-not (Test-Path -LiteralPath $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}
$buildDirPath = (Resolve-Path -LiteralPath $BuildDir).Path
$tempCtestDir = Join-Path $buildDirPath "TemporaryDrawCTest"
New-Item -ItemType Directory -Path $tempCtestDir -Force | Out-Null

$ctestFile = Join-Path $tempCtestDir "CTestTestfile.cmake"
$scriptPath = ConvertTo-CMakePath (Join-Path $repoRootPath "scripts/run_draw_smoke.ps1")
$repoCmakePath = $repoRootPath -replace '\\', '/'
$buildCmakePath = $buildDirPath -replace '\\', '/'
$reproCmakePath = ConvertTo-CMakePath $ReproTcl

$content = @"
add_test([=[$TestName]=]
  powershell
  -NoProfile
  -ExecutionPolicy Bypass
  -File [=[$scriptPath]=]
  -RepoRoot [=[$repoCmakePath]=]
  -BuildDir [=[$buildCmakePath]=]
  -Config [=[$Config]=]
  -TestName [=[$TestName]=]
  -TclScript [=[$reproCmakePath]=]
  -SuccessToken [=[$SuccessToken]=]
)
set_tests_properties([=[$TestName]=] PROPERTIES
  LABELS "draw;repro;temporary;occt"
  FAIL_REGULAR_EXPRESSION "DRAW_SMOKE_FAILED|DRAW_SMOKE_ERROR|Faulty"
)
"@

Set-Content -LiteralPath $ctestFile -Value $content -Encoding UTF8

[ordered]@{
    test_name = $TestName
    ctest_dir = $tempCtestDir
    ctest_file = $ctestFile
    command = "ctest --test-dir `"$tempCtestDir`" -R $TestName --output-on-failure"
} | ConvertTo-Json -Depth 4
