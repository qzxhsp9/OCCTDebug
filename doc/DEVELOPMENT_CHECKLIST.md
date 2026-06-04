# OCCTDebug 开发验收清单

更新时间：2026-06-04

## 1. 项目上下文

- [x] 根目录存在 `AGENTS.md`。
- [x] `doc/roadmap2.md` 存在并作为当前主路线图维护。
- [x] `doc/roadmap1.md` / `doc/roadmap.md` 若存在，仅作为历史参考，不作为当前开发依据。
- [x] `doc/OCCT_AutoFix_Workbench_Design.md` 存在。
- [x] `doc/OCCT_Kernel_Expert_Workbench_UI_Design.md` 存在。
- [x] `doc/CODEX_CONTEXT.md` 存在。
- [x] `doc/CODEX_TASKS.md` 存在。
- [x] `doc/DRAW_SMOKE_CTEST.md` 存在。

## 2. 基础工程

- [x] 项目可以在 Windows x64 下构建。
- [x] Qt 路径通过本地 `src/QtWorkbenchDefaults.cmake` 配置，不硬编码到源码。
- [x] OCCT 路径通过仓库 `depends/occt` 和 CMake 脚本解析。
- [x] 有 `scripts/verify_env.ps1`。
- [ ] 有 `scripts/build.ps1`。
- [ ] 有 `scripts/run.ps1`。
- [x] 有等价 CMake configure/build/ctest 运行方式。

## 3. UI 骨架

- [x] 顶部工具栏包含：问题录入、复现生成、源码分析、补丁方案、回归验证、知识归档。
- [x] 左侧包含案例列表、流程状态、关键输入。
- [x] 中央包含源码、复现脚本、几何视图、差异对比、环境信息标签页。
- [x] 右侧包含诊断结论、候选补丁、验证结果、相似案例。
- [x] 底部包含 DRAW 控制台、PowerShell/CMake、testgrid 结果。
- [x] 深色主题统一，控件层次清晰。
- [ ] 主要面板完成可维护拆分，避免 `WorkbenchWindow.cpp` 继续膨胀。

## 4. Case 数据模型

- [x] CaseRecord / CaseManifest 支持 JSON 加载基础。
- [x] EnvironmentSnapshot 已有 JSON 输出脚本基础。
- [x] ReproPack 已有脚本级导出基础。
- [x] EvidenceBundle 已有面板/模型骨架。
- [x] PatchCandidate 已有 diff、风险、影响模块和审查流程骨架。
- [x] VerificationReport 已有 testgrid/testdiff 结果解析骨架。
- [ ] 上述模型统一落盘到真实 `cases/<case_id>/` workspace。

## 5. 自动化执行

- [x] 命令执行器已有记录命令、cwd、stdout、stderr、退出码和耗时的基础能力。
- [ ] 支持取消运行中任务。
- [x] 命令输出已有接入底部控制台的基础。
- [ ] 命令执行默认限制在 Case workspace。
- [x] 失败时有明确错误提示的基础。

## 6. OCCT 集成

- [x] 能识别仓库内 OCCT 安装路径。
- [x] 能识别 DRAWEXE。
- [x] 能通过 CTest 运行简单 DRAW 脚本。
- [x] 能通过 CTest 运行最小 `checkshape` smoke。
- [x] 能解析基本 checkshape 输出。
- [x] 能保存 DRAW 运行日志。
- [x] 已有 testgrid/testdiff 结果解析骨架。
- [ ] testgrid/testdiff runner 正式接入 UI 和 Case。

## 7. 报告与归档

- [x] 已有 Markdown 报告生成骨架。
- [ ] 报告包含真实 Case 的环境、复现、证据、结论、验证和风险。
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
