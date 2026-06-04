param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$OutputPath = ""
)

$ErrorActionPreference = "Stop"

function Get-CommandSummary {
    param([string]$Name)

    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if (-not $cmd) {
        return [ordered]@{
            found = $false
            path = ""
            version = ""
        }
    }

    $version = ""
    try {
        if ($Name -eq "cmake") {
            $versionLine = (& $cmd.Source --version 2>$null | Select-Object -First 1)
            $version = [string]$versionLine
        }
    } catch {
        $version = ""
    }

    return [ordered]@{
        found = $true
        path = [string]$cmd.Source
        version = $version
    }
}

function Find-FirstExistingPath {
    param([string[]]$Candidates)

    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate)) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    return ""
}

function Find-DrawExe {
    param([string]$Root)

    $candidatePatterns = @(
        "depends/occt/lib/Debug/bind/DRAWEXE.exe",
        "depends/occt/lib/Release/bin/DRAWEXE.exe",
        "depends/occt/lib/RelWithDebInfo/bini/DRAWEXE.exe"
    )

    foreach ($pattern in $candidatePatterns) {
        $candidate = Join-Path $Root $pattern
        if (Test-Path -LiteralPath $candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    $found = Get-ChildItem -Path (Join-Path $Root "depends/occt") -Recurse -Filter DRAWEXE.exe -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($found) {
        return $found.FullName
    }

    return ""
}

function Get-VisualStudioSummary {
    $vswhereCandidates = @(
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\Installer\vswhere.exe"
    )
    $vswhere = Find-FirstExistingPath $vswhereCandidates

    if ($vswhere) {
        try {
            $json = & $vswhere -latest -format json -products * 2>$null | ConvertFrom-Json
            if ($json) {
                return [ordered]@{
                    found = $true
                    display_name = [string]$json.displayName
                    installation_version = [string]$json.installationVersion
                    installation_path = [string]$json.installationPath
                    vswhere = $vswhere
                }
            }
        } catch {
        }
    }

    $devCmd = Find-FirstExistingPath @(
        "D:\Programming\VisualStudio\2026\Community\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"
    )

    return [ordered]@{
        found = [bool]$devCmd
        display_name = ""
        installation_version = ""
        installation_path = ""
        developer_command = $devCmd
    }
}

$repoRootPath = (Resolve-Path -LiteralPath $RepoRoot).Path
$occtRoot = Join-Path $repoRootPath "depends/occt"
$freetypeRoot = Join-Path $repoRootPath "depends/occt_3rdparty/freetype-2.13.3-x64"
$tcltkRoot = Join-Path $repoRootPath "depends/occt_3rdparty/tcltk-8.6.15-x64"
$qtDefaultFile = Join-Path $repoRootPath "src/QtWorkbenchDefaults.cmake"
$drawExe = Find-DrawExe $repoRootPath

$qtRoot = ""
if (Test-Path -LiteralPath $qtDefaultFile) {
    $qtLine = Get-Content -LiteralPath $qtDefaultFile -ErrorAction SilentlyContinue |
        Where-Object { $_ -match 'OCCTDEBUG_QT_DEFAULT_KIT\s+"([^"]+)"' } |
        Select-Object -First 1
    if ($qtLine -match 'OCCTDEBUG_QT_DEFAULT_KIT\s+"([^"]+)"') {
        $qtRoot = $Matches[1]
    }
}
if (-not $qtRoot -and $env:QTDIR) {
    $qtRoot = $env:QTDIR
}

$os = Get-CimInstance Win32_OperatingSystem
$cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
$cmake = Get-CommandSummary cmake

$snapshot = [ordered]@{
    schema_version = 1
    generated_at = (Get-Date).ToString("o")
    repo = [ordered]@{
        root = $repoRootPath
    }
    windows = [ordered]@{
        caption = [string]$os.Caption
        version = [string]$os.Version
        build_number = [string]$os.BuildNumber
        architecture = [string]$os.OSArchitecture
    }
    hardware = [ordered]@{
        cpu = [string]$cpu.Name
        logical_processors = [int]$cpu.NumberOfLogicalProcessors
        total_memory_gb = [math]::Round($os.TotalVisibleMemorySize / 1MB, 2)
    }
    visual_studio = Get-VisualStudioSummary
    cmake = $cmake
    qt = [ordered]@{
        root = [string]$qtRoot
        exists = [bool]($qtRoot -and (Test-Path -LiteralPath $qtRoot))
        defaults_file = $qtDefaultFile
    }
    occt = [ordered]@{
        root = $occtRoot
        exists = [bool](Test-Path -LiteralPath $occtRoot)
        drawexe = $drawExe
        drawexe_exists = [bool]$drawExe
        draw_resources = Join-Path $occtRoot "src/DrawResources"
        draw_default_exists = [bool](Test-Path -LiteralPath (Join-Path $occtRoot "src/DrawResources/DrawDefault"))
    }
    freetype = [ordered]@{
        root = $freetypeRoot
        exists = [bool](Test-Path -LiteralPath $freetypeRoot)
    }
    tcltk = [ordered]@{
        root = $tcltkRoot
        exists = [bool](Test-Path -LiteralPath $tcltkRoot)
        bin = Join-Path $tcltkRoot "bin"
        lib = Join-Path $tcltkRoot "lib"
        tcl86_dll_exists = [bool](Test-Path -LiteralPath (Join-Path $tcltkRoot "bin/tcl86.dll"))
        tk86_dll_exists = [bool](Test-Path -LiteralPath (Join-Path $tcltkRoot "bin/tk86.dll"))
    }
    environment = [ordered]@{
        CASROOT = [string]$env:CASROOT
        QTDIR = [string]$env:QTDIR
        PATH_summary = (($env:PATH -split ';' | Where-Object { $_ } | Select-Object -First 12) -join ';')
    }
}

$json = $snapshot | ConvertTo-Json -Depth 8

if ($OutputPath) {
    $parent = Split-Path -Parent $OutputPath
    if ($parent -and -not (Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Path $parent | Out-Null
    }
    Set-Content -LiteralPath $OutputPath -Value $json -Encoding UTF8
}

$json
