# OCCTDebug Codex 项目上下文摘要

更新时间：2026-06-04

## 1. 一句话目标

OCCTDebug 是 Windows 本地的“OCCT 内核专家工作台”：把用户的 OCCT 问题转化为可复现 Case、环境快照、DRAW/C++ 复现、证据链、源码定位、候选补丁、回归验证和知识沉淀。

项目不做聊天式问答工具，而做可审查、可运行、可归档的工程工作台。

## 2. 当前实现基线

当前仓库已经从旧代码重搭为新的 Qt Widgets 工作台，并完成以下基础能力：

- Qt 主窗口骨架：顶部工具栏、左侧 Case/流程/输入区、中央源码/复现/几何/差异/环境标签页、右侧诊断/补丁/验证/相似案例、底部 DRAW/PowerShell-CMake/testgrid 控制台。
- Case 数据模型：`CaseManifest` 已支持 JSON 加载和 mock/sample 数据驱动 UI。
- 环境采集：`scripts/verify_env.ps1` 可输出 Windows、VS、CMake、Qt、OCCT、Tcl/Tk、FreeType、DRAWEXE 与 DRAW CTest 结果。
- 命令执行：`CommandRunner` 已提供统一外部命令执行基础。
- DRAW 基础闭环：UI 已有 DRAW 脚本编辑/运行入口；CTest 已有 `draw_smoke` 和 `draw_checkshape_smoke`。
- DRAW 运行辅助：已支持 DRAW 日志解析、临时 repro Tcl 注册为 CTest、失败 case 导出 Repro Pack。
- OCCT Viewer：已有基础 Qt/OCCT Viewer 集成入口。
- Evidence / Report / Verify / Source / Knowledge / Patch：已有第一版模型或解析器骨架，用于后续串成真实 Case 闭环。

当前重点不再是“继续堆静态 UI”，而是把已有骨架收束成真实 Case 工作流。

## 3. 产品最终形态

最终界面是工程化桌面工作台：

- 左侧：案例列表、状态流转、关键输入。
- 中央：源码定位、复现脚本、几何视图、证据链、差异对比、环境信息。
- 右侧：诊断结论、候选补丁、验证结果、相似案例。
- 底部：DRAW 控制台、PowerShell/CMake 控制台、testgrid 结果。

工作台强调“简洁、清晰、实用、高效”，给 OCCT 内核工程师提供可审查的调试闭环。

## 4. 核心闭环

```text
问题录入
  -> Case workspace 创建
  -> 环境采集
  -> DRAW / C++ 复现
  -> 日志与 checkshape 解析
  -> 几何查看与证据收集
  -> 源码定位与相似案例检索
  -> 根因分析
  -> 候选补丁
  -> 回归验证
  -> 报告生成
  -> 知识归档
```

每一步都应产出结构化制品，最终形成可复用的 `Case` 归档目录。

## 5. 核心对象模型

### CaseRecord / CaseManifest

记录 case id、标题、状态、问题类型、OCCT 版本、输入文件、复现方式、保密级别、证据和报告索引。

### EnvironmentSnapshot

记录 Windows、Visual Studio、CMake、Qt、OCCT、Tcl/Tk、FreeType、DRAWEXE、CTest smoke 结果、PATH 摘要和关键环境变量。

### ReproPack

包含输入数据索引、DRAW/Tcl 脚本、C++ 最小复现、运行脚本、stdout/stderr、result JSON 和 manifest。

### EvidenceBundle

包含调用栈、DRAW 日志、checkshape 结果、Shape dump、拓扑统计、截图、相似 issue/test/case 和相关源码。

### PatchCandidate

包含 diff、影响模块、风险等级、新增测试、验证状态和人工审查意见。

### VerificationReport

包含原始问题复验、相关测试组、CTest、testgrid/testdiff、性能变化、失败项和回归风险。

## 6. 当前 MVP 边界

近期 MVP 不追求自动修复 OCCT，而追求第一个可演示、可验证的本地闭环：

1. 创建或加载一个 sample Case。
2. 采集环境并保存到 Case artifacts。
3. 在 UI 中编辑并运行 DRAW 脚本。
4. 解析 DRAW 日志和 checkshape 结果。
5. 把日志、环境、复现脚本组成 Evidence。
6. 生成 Markdown 报告。
7. 导出 Repro Pack。

第二阶段再扩大到真实模型导入、源码跳转、testgrid/testdiff、候选补丁审查和知识库。

## 7. 设计约束

- Windows x64 only。
- Qt Widgets 优先。
- OCCT、Qt、DRAWEXE、testgrid 路径必须可配置，不写死个人机器路径到源码。
- 所有自动执行命令必须保留命令、cwd、stdout、stderr、退出码、耗时和日志路径。
- `draw_smoke` 是后续 testgrid/testdiff 的前置环境门禁。
- 所有补丁建议必须关联复现和验证结果。
- 私有 CAD 数据默认不得外发或提交。
- 文档和计划以 `doc/roadmap2.md` 为准；旧路线图只作历史参考。

## 8. 推荐下一开发目标

下一步应实现“Case workspace 最小闭环”，而不是继续扩展静态页面：

1. 引入可提交的默认配置和本地配置模板。
2. 建立 `AppContext` / `ConfigService`，统一路径和服务生命周期。
3. 建立 `cases/<case_id>/` 工作区结构。
4. 将 UI mock 数据切换为 sample Case 数据绑定。
5. 将 DRAW UI 运行结果写入 Case artifacts。
6. 将 DRAW 日志解析结果进入 Evidence 面板。
7. 生成第一份真实 Case Markdown 报告和 Repro Pack。
