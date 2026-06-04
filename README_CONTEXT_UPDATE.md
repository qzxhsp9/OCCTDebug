# OCCTDebug 上下文更新包使用说明

把本目录中的文件复制到 OCCTDebug 仓库根目录：

```powershell
Copy-Item .\AGENTS.md <OCCTDebug>\AGENTS.md -Force
Copy-Item .\.codex <OCCTDebug>\.codex -Recurse -Force
Copy-Item .\doc\* <OCCTDebug>\doc\ -Recurse -Force
Copy-Item .\scripts\update_codex_context.ps1 <OCCTDebug>\scripts\update_codex_context.ps1 -Force
```

然后进入仓库根目录：

```powershell
cd <OCCTDebug>
powershell -ExecutionPolicy Bypass -File .\scripts\update_codex_context.ps1
git add AGENTS.md .codex/config.toml doc/CODEX_CONTEXT.md doc/CODEX_TASKS.md doc/DEVELOPMENT_CHECKLIST.md scripts/update_codex_context.ps1
git commit -m "docs: add Codex project context for OCCTDebug"
```

在 Codex 中打开 OCCTDebug 后，建议先发起以下任务：

```text
请先阅读 AGENTS.md、doc/roadmap.md、doc/CODEX_CONTEXT.md、doc/CODEX_TASKS.md 和 doc/OCCT_Kernel_Expert_Workbench_UI_Design.md。
任务：检查当前 OCCTDebug 仓库结构与路线图的差距，只输出最小下一步开发计划，不要修改代码。
```
