# OCCTDebug Codex 任务模板

更新时间：2026-06-04

本文件用于给 Codex 拆分任务。每次任务必须小而可验证，遵循 AGENTS.md 的 PDCA：先计划、再执行、后验证、再沉淀。

## 1. 当前进度摘要

已完成或已有基础实现：

- T0：项目结构只读分析。
- T1：Qt 主窗口骨架。
- T2：Case 数据模型与 JSON 加载。
- T3：Mock 数据驱动 UI。
- T4：环境采集 `verify_env.ps1`。
- T5：统一 `CommandRunner`。
- T6：DRAW/Tcl 复现脚本编辑与运行入口。
- T7：OCCT Viewer 基础集成。
- T8：Evidence 证据链面板骨架。
- T9：Markdown 报告生成骨架。
- T10：testgrid/testdiff 结果解析骨架。
- T11：源码索引与相似案例检索骨架。
- T12：候选补丁与审查流程骨架。
- DRAW smoke CTest：`draw_smoke`、`draw_checkshape_smoke`。
- DRAW 辅助脚本：日志解析、临时 CTest 注册、Repro Pack 导出。

后续任务应优先把这些基础能力串成真实 Case 工作流。

## 2. 推荐任务顺序

```text
N1 配置系统与 AppContext
N2 Case workspace 创建/加载/保存
N3 UI 从 mock 数据切换到 sample Case 数据绑定
N4 DRAW UI 运行结果写入 Case artifacts
N5 DRAW 日志解析结果进入 Evidence 面板
N6 环境采集结果进入 Case artifacts 与环境 tab
N7 生成真实 Case Markdown 报告
N8 导出失败 DRAW Repro Pack
N9 OCCT Viewer 加载 BREP/STEP
N10 testgrid/testdiff runner 与 draw_smoke 前置门禁
N11 源码跳转、相似案例、诊断报告
N12 候选补丁、人工审查、验证报告
```

## 3. N1：配置系统与 AppContext

```text
请先阅读 AGENTS.md、doc/CODEX_CONTEXT.md、doc/roadmap2.md。

任务：实现 OCCTDebug 的最小配置系统与 AppContext。

范围：
- 新增 config/workbench.default.yaml。
- 新增 config/workbench.local.example.yaml。
- 确认 config/workbench.local.yaml、cases/、artifacts/、knowledge/cache/ 被 gitignore。
- 实现 ConfigService，读取默认配置、本地配置和 CMake 推导路径。
- 实现 AppContext，集中持有 repo root、build root、case root、OCCT/Qt/DRAW 路径和服务对象。

不要做：
- 不实现复杂设置 UI。
- 不引入新第三方依赖。
- 不写死个人机器路径到源码。

验收：
- CMake configure/build 通过。
- 应用启动通过。
- 配置缺失时有明确 fallback 或错误提示。
- 文档更新配置说明。
```

## 4. N2：Case Workspace 创建/加载/保存

```text
任务：实现最小 Case workspace。

范围：
- 定义 cases/<case_id>/ 标准目录。
- 支持创建 sample case。
- 支持读取/保存 case.json。
- 保存 repro、env、logs、artifacts、report 的相对路径索引。

不要做：
- 不导入大型 CAD 文件。
- 不实现完整 case pack。
- 不实现自动诊断。

验收：
- 能创建一个 sample case。
- 关闭再打开后能恢复 case 基本信息。
- 所有路径优先使用相对路径。
```

## 5. N3：UI 数据绑定

```text
任务：将 WorkbenchWindow 静态 mock 数据逐步替换为 sample Case 数据。

范围：
- 左侧案例列表由 CaseManifest 驱动。
- 流程状态由 case status 驱动。
- 中央复现脚本、环境信息、证据列表从 case artifacts 读取。
- 右侧诊断/补丁/验证保留 mock，但与当前 case 关联。

不要做：
- 不做大规模 UI 重构。
- 不一次性拆完所有面板。

验收：
- 切换 case 后关键 UI 区域刷新。
- 没有 case 时 UI 显示空状态。
- 构建和启动检查通过。
```

## 6. N4：DRAW UI 运行结果落盘

```text
任务：完善 UI 中运行 DRAW 脚本的闭环。

范围：
- UI 调用现有 DRAW runner / 脚本能力。
- stdout、stderr、result.json 写入当前 case artifacts。
- 底部 DRAW 控制台实时追加输出。
- 失败时显示日志路径和错误摘要。

验收：
- UI 可运行一个最小 repro.tcl。
- 运行结果能在 case 目录中找到。
- 失败和成功都有结构化记录。
```

## 7. N5：DRAW 日志解析与 Evidence 接入

```text
任务：把 DRAW 运行日志解析结果接入 Evidence 面板。

范围：
- 复用 scripts/parse_draw_log.ps1 或 C++ 解析器。
- 结构化展示 success token、错误行、checkshape.status。
- Evidence 项可跳转到原始日志位置。

验收：
- checkshape valid/faulty/unknown 可区分。
- 错误日志能进入 Evidence。
- 原始日志不丢失。
```

## 8. N6：环境采集报告接入 Case

```text
任务：把 verify_env.ps1 输出保存到当前 Case，并在环境 tab 展示。

范围：
- UI 触发环境采集。
- env_snapshot.json 写入 case/env/。
- 展示 OCCT、DRAWEXE、Qt、CMake、VS、DRAW smoke 状态。

验收：
- 本地配置正确时 DRAW smoke 状态显示通过。
- DRAWEXE 缺失时显示清晰错误。
```

## 9. N7：真实 Case Markdown 报告

```text
任务：生成当前 Case 的 Markdown 报告。

范围：
- 报告包含问题摘要、环境、复现脚本、运行结果、Evidence、checkshape、已知限制。
- 输出到 case/report/report.md。
- 避免暴露敏感绝对路径。

验收：
- 报告可重复生成。
- 报告引用的 artifact 路径有效。
```

## 10. N8：Repro Pack 导出

```text
任务：将失败 DRAW case 打包为 Repro Pack。

范围：
- 复用 scripts/export_repro_pack.ps1。
- 包含 repro.tcl、stdout/stderr、result.json、env_snapshot.json、manifest.json。
- 默认不复制大型模型文件，除非用户明确选择。

验收：
- 导出的 pack 可独立说明如何复现。
- manifest 不包含不必要的个人绝对路径。
```

## 11. N9：OCCT Viewer 加载模型

```text
任务：让 OCCT Viewer 从基础集成进入可用状态。

范围：
- 至少支持加载 BREP 或 STEP。
- 支持 fit all、基础显示、清空视图。
- 不实现复杂选择和修复。

验收：
- sample 小模型可加载显示。
- 文件不存在或格式错误时 UI 不崩溃。
```

## 12. N10：testgrid/testdiff 接入

```text
任务：实现 testgrid/testdiff 的最小 runner。

范围：
- 所有 testgrid/testdiff 测试前要求 draw_smoke 通过。
- 支持运行一个配置好的最小测试命令。
- 解析通过率、失败列表、日志路径。

不要做：
- 不接入全量大型回归。
- 不要求联网。

验收：
- draw_smoke 失败时不启动 testgrid。
- 结果能显示到底部 testgrid 面板。
```

## 13. Codex 代码审查任务

```text
请审查当前未提交修改。

重点检查：
1. 是否偏离 AGENTS.md 的项目目标。
2. 是否继续保持 UI 与业务数据模型分离。
3. Windows 路径、Qt 对象生命周期、异步命令执行是否有问题。
4. 是否有私有数据泄露风险。
5. 是否有缺失的构建/CTest/手动验证步骤。
6. 文档是否同步更新。

要求：
- 只输出可操作审查意见。
- 不直接修改代码。
```
