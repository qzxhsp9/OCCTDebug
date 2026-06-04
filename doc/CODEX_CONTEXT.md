# OCCTDebug Codex 项目上下文摘要

## 1. 一句话目标

OCCTDebug 要做成一个 Windows 本地的“OCCT 内核专家工作台”：把用户的 OCCT 问题转化为可复现案例、证据链、源码定位、候选补丁、回归验证和知识沉淀。

## 2. 产品最终形态

最终界面不是普通聊天工具，而是工程化桌面工作台：

- 左侧：案例列表、状态流转、关键输入。
- 中央：源码定位、复现脚本、几何视图、证据链、差异对比、环境信息。
- 右侧：诊断结论、候选补丁、验证结果、相似案例。
- 底部：DRAW 控制台、PowerShell/CMake 控制台、testgrid 结果。

工作台强调“简洁、清晰、实用、高效”，给 OCCT 内核工程师提供可审查的调试闭环。

## 3. 关键闭环

```text
问题录入
  -> 环境采集
  -> 自动复现
  -> 最小化数据
  -> 证据收集
  -> 根因分析
  -> 补丁生成
  -> 回归验证
  -> 知识归档
```

每一步都应产出结构化制品，最终形成 `Case` 归档目录。

## 4. 核心对象模型建议

### CaseRecord

```yaml
case_id: OCC-LOCAL-2026-0001
title: Fillet 更新失败，Null curve 导致崩溃
status: analyzing
occt_version: 7.8.1
toolchain: VS2022 x64
input_model: valve_body_min.brep
repro_type: DRAW script
failure_mode: crash / wrong result / invalid shape / performance / import-export
confidentiality: internal
```

### EnvironmentSnapshot

记录 Windows、Visual Studio、CMake、Qt、OCCT、Tcl/Tk、PATH 摘要、关键环境变量和构建配置。

### ReproPack

包含输入数据、DRAW/Tcl 脚本、C++ 最小复现、运行脚本、日志和复现结果。

### EvidenceBundle

包含调用栈、日志、ASan 报告、checkshape 结果、Shape Dump、拓扑统计、截图、相似 issue/test/case。

### PatchCandidate

包含 diff、影响模块、风险等级、新增测试、验证状态和人工审查意见。

### VerificationReport

包含原始问题复验、相关测试组、testgrid/testdiff、性能变化、失败项和回归风险。

## 5. 初期 MVP 边界

第一阶段不追求真正“自动修复全部 OCCT 问题”，而是先把专家工作台的基本骨架打通：

1. Qt 主界面和深色主题。
2. Case 列表和流程状态。
3. mock 数据驱动的源码、脚本、几何、证据、补丁、验证面板。
4. 环境采集脚本。
5. 本地命令执行器。
6. 报告生成框架。

第二阶段再接入真实 OCCT：DRAWEXE、CMake、MSBuild、testgrid、testdiff、OCCT 源码索引。

## 6. 设计约束

- Windows only。
- Qt 优先。
- OCCT 路径和 Qt 路径必须可配置。
- 所有自动执行命令必须保留日志。
- 所有补丁建议必须关联复现和测试。
- 私有 CAD 数据默认不得外发或提交。
- AGENTS.md 保持简洁，详细任务放到 `doc/CODEX_TASKS.md`。

## 7. 推荐当前首个开发目标

构建一个能运行的 Qt 桌面应用骨架：

- 顶部工具栏：问题录入、复现生成、源码分析、补丁方案、回归验证、知识归档。
- 左侧：案例列表、流程状态、关键输入。
- 中央：源码编辑器 mock、DRAW 脚本、3D 几何视图 placeholder、几何检查表。
- 右侧：诊断结论、候选补丁、验证结果、相似案例。
- 底部：三类控制台 tab。

完成后再逐步替换 mock 数据为真实 Case 数据模型。
