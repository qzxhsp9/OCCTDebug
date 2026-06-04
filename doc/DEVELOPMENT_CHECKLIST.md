# OCCTDebug 开发验收清单

## 1. 项目上下文

- [ ] 根目录存在 `AGENTS.md`。
- [ ] `doc/roadmap.md` 存在并是最新路线图。
- [ ] `doc/OCCT_AutoFix_Workbench_Design.md` 存在。
- [ ] `doc/OCCT_Kernel_Expert_Workbench_UI_Design.md` 存在。
- [ ] `doc/CODEX_CONTEXT.md` 存在。
- [ ] `doc/CODEX_TASKS.md` 存在。

## 2. 基础工程

- [ ] 项目可以在 Windows x64 下构建。
- [ ] Qt 路径可配置，不硬编码到源码。
- [ ] OCCT 路径可配置，不硬编码到源码。
- [ ] 有 `scripts/verify_env.ps1`。
- [ ] 有 `scripts/build.ps1`。
- [ ] 有 `scripts/run.ps1` 或等价运行方式。

## 3. UI 骨架

- [ ] 顶部工具栏包含：问题录入、复现生成、源码分析、补丁方案、回归验证、知识归档。
- [ ] 左侧包含案例列表、流程状态、关键输入。
- [ ] 中央包含源码、复现脚本、几何视图、差异对比、环境信息标签页。
- [ ] 右侧包含诊断结论、候选补丁、验证结果、相似案例。
- [ ] 底部包含 DRAW 控制台、PowerShell/CMake、testgrid 结果。
- [ ] 深色主题统一，控件层次清晰。

## 4. Case 数据模型

- [ ] CaseRecord 支持 JSON 保存/加载。
- [ ] EnvironmentSnapshot 支持 JSON 保存/加载。
- [ ] ReproPack 支持输入文件、脚本、运行结果索引。
- [ ] EvidenceBundle 支持日志、调用栈、几何检查、相似案例。
- [ ] PatchCandidate 支持 diff、风险、影响模块和测试状态。
- [ ] VerificationReport 支持 testgrid/testdiff 结果摘要。

## 5. 自动化执行

- [ ] 命令执行器记录命令、cwd、stdout、stderr、退出码和耗时。
- [ ] 支持取消运行中任务。
- [ ] 命令输出可实时显示到底部控制台。
- [ ] 命令执行默认限制在 Case workspace。
- [ ] 失败时有明确错误提示。

## 6. OCCT 集成

- [ ] 能识别 OCCT 安装/源码路径。
- [ ] 能识别 DRAWEXE。
- [ ] 能运行简单 DRAW 脚本。
- [ ] 能解析基本 checkshape 输出。
- [ ] 能保存复现日志。
- [ ] 后续接入 testgrid/testdiff。

## 7. 报告与归档

- [ ] 能生成 Markdown 报告。
- [ ] 报告包含环境、复现、证据、结论、验证和风险。
- [ ] 私有路径和敏感文件名可脱敏。
- [ ] Case artifacts 目录结构稳定。

## 8. 质量门禁

每次合并前至少确认：

- [ ] 修改范围可解释。
- [ ] 构建或最小验证已运行。
- [ ] 没有提交临时文件、CAD 私有数据、dump、PDB、大型 build 产物。
- [ ] 没有无关格式化大改。
- [ ] UI 代码与业务逻辑没有严重耦合。
- [ ] README 或 doc 已同步更新。
