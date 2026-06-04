<#
.SYNOPSIS
  检查 OCCTDebug 仓库中的 Codex 项目上下文文件是否齐全。

.USAGE
  在 OCCTDebug 仓库根目录运行：
    powershell -ExecutionPolicy Bypass -File .\scripts\update_codex_context.ps1

.NOTES
  该脚本不会联网，也不会修改 OCCT 或 Qt 安装目录。
#>

$ErrorActionPreference = "Stop"

function Test-FileRequired($Path, $Label) {
    if (Test-Path $Path) {
        Write-Host "[OK] $Label -> $Path" -ForegroundColor Green
        return $true
    } else {
        Write-Host "[MISSING] $Label -> $Path" -ForegroundColor Yellow
        return $false
    }
}

Write-Host "== OCCTDebug Codex Context Check ==" -ForegroundColor Cyan
Write-Host "Repo: $(Get-Location)"

$ok = $true
$ok = (Test-FileRequired "AGENTS.md" "Codex project instructions") -and $ok
$ok = (Test-FileRequired ".codex\config.toml" "Codex project config") -and $ok
$ok = (Test-FileRequired "doc\roadmap.md" "Roadmap") -and $ok
$ok = (Test-FileRequired "doc\CODEX_CONTEXT.md" "Codex context summary") -and $ok
$ok = (Test-FileRequired "doc\CODEX_TASKS.md" "Codex task templates") -and $ok
$ok = (Test-FileRequired "doc\DEVELOPMENT_CHECKLIST.md" "Development checklist") -and $ok

$uiDocExists = (Test-Path "doc\OCCT_Kernel_Expert_Workbench_UI_Design.md") -or (Get-ChildItem -Path "doc" -Filter "*UI*Design*.md" -ErrorAction SilentlyContinue)
if ($uiDocExists) {
    Write-Host "[OK] UI design document found" -ForegroundColor Green
} else {
    Write-Host "[MISSING] UI design document under doc\" -ForegroundColor Yellow
    $ok = $false
}

$designDocExists = (Test-Path "doc\OCCT_AutoFix_Workbench_Design.md") -or (Get-ChildItem -Path "doc" -Filter "*AutoFix*Design*.md" -ErrorAction SilentlyContinue)
if ($designDocExists) {
    Write-Host "[OK] Tool design document found" -ForegroundColor Green
} else {
    Write-Host "[MISSING] Tool design document under doc\" -ForegroundColor Yellow
    $ok = $false
}

Write-Host ""
if ($ok) {
    Write-Host "Context files are ready for Codex." -ForegroundColor Green
    Write-Host "Recommended first Codex prompt:"
    Write-Host "请先阅读 AGENTS.md、doc/roadmap.md、doc/CODEX_CONTEXT.md 和 UI 设计文档，检查当前工程结构并给出下一步最小开发任务。" -ForegroundColor Cyan
    exit 0
} else {
    Write-Host "Some context files are missing. Please copy the provided context package into the repository root, then rerun this script." -ForegroundColor Yellow
    exit 1
}
