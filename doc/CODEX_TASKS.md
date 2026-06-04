# OCCTDebug Codex 任务模板

本文件用于给 Codex 拆分任务。每次任务尽量小而可验证，不要一次性要求完成全部系统。

## 1. 初始化项目上下文任务

```text
请先阅读 AGENTS.md、doc/roadmap.md、doc/CODEX_CONTEXT.md、doc/OCCT_Kernel_Expert_Workbench_UI_Design.md。
任务：检查当前仓库结构是否符合 OCCTDebug 的目标，并给出最小改动方案。
要求：不要修改代码，先输出目录现状、缺失项、建议的下一步任务列表。
```

## 2. 建立 Qt 主窗口骨架

```text
请阅读 AGENTS.md 和 doc/CODEX_CONTEXT.md。
任务：实现 OCCTDebug 的 Qt 主窗口骨架。
范围：顶部工具栏、左侧案例/流程区、中央工作区、右侧诊断区、底部控制台区。
要求：
1. 可编译运行。
2. 使用 mock 数据。
3. 代码分层，不要把所有 UI 逻辑塞进 main.cpp。
4. 提供构建命令和运行说明。
```

## 3. 实现 Case 数据模型

```text
任务：设计并实现 CaseRecord、EnvironmentSnapshot、ReproPack、EvidenceBundle、PatchCandidate、VerificationReport 的基础 C++/Qt 数据模型。
要求：
1. 支持 JSON 序列化/反序列化。
2. 支持从 sample case JSON 加载到 UI。
3. 提供一个 tests 或 demo 数据文件。
4. 不接入真实 OCCT，只做数据模型与 UI 绑定。
```

## 4. 环境采集器

```text
任务：实现 Windows 环境采集模块和 verify_env.ps1。
采集项：Windows 版本、CPU/内存、Visual Studio、MSVC、CMake、Qt、OCCT 路径、DRAWEXE 可用性、关键环境变量。
要求：
1. 输出 JSON。
2. UI 能显示采集结果。
3. 命令失败时不能崩溃，应显示诊断信息。
```

## 5. 命令执行器

```text
任务：实现统一 CommandRunner，用于执行 PowerShell/CMake/DRAW/testgrid 命令。
要求：
1. 记录命令、工作目录、环境变量摘要、stdout/stderr、退出码、耗时。
2. 支持实时输出到底部控制台。
3. 支持取消任务。
4. 所有命令默认在当前 Case workspace 内执行。
```

## 6. DRAW 复现脚本管理

```text
任务：实现 DRAW/Tcl 复现脚本编辑与运行入口。
要求：
1. UI 能展示和编辑 repro.tcl。
2. 能调用配置的 DRAWEXE 执行脚本。
3. 能收集日志与退出状态。
4. 运行结果写入当前 Case 的 artifacts 目录。
```

## 7. 几何视图占位到 OCCT Viewer

```text
任务：将当前几何视图 placeholder 替换为 OCCT 3D Viewer 的基础集成。
要求：
1. 能加载 BREP/STEP 中至少一种格式。
2. 支持显示、fit all、选择边/面、基础高亮。
3. 不实现复杂分析，只提供后续扩展接口。
```

## 8. 证据链面板

```text
任务：实现 Evidence 面板，包括调用栈、日志摘要、checkshape 结果、Shape 统计、相似案例列表。
要求：
1. 从 JSON/mock 数据加载。
2. 支持点击证据跳转到源码行或日志位置。
3. 不需要联网。
```

## 9. 报告生成

```text
任务：生成当前 Case 的 Markdown 报告。
报告包括：问题摘要、环境、输入、复现步骤、失败证据、根因结论、候选补丁、验证结果、风险。
要求：
1. 输出到 case/artifacts/report.md。
2. UI 有“打开报告”入口。
3. 报告不得包含敏感绝对路径，必要时做脱敏。
```

## 10. Codex 代码审查任务

```text
请审查当前未提交修改。
重点检查：
1. 是否偏离 AGENTS.md 的项目目标。
2. UI 是否过度复杂或难维护。
3. Windows 路径、Qt 对象生命周期、线程/异步命令执行是否有问题。
4. 是否有私有数据泄露风险。
5. 是否有缺失的构建/验证步骤。
请输出可操作的审查意见，不要直接修改代码。
```
