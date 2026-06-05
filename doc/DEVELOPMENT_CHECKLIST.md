# OCCTDebug 开发验收清单

更新时间：2026-06-05

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
- [x] 有 `scripts/build.ps1`。
- [x] 有 `scripts/run.ps1`。
- [x] 有等价 CMake configure/build/ctest 运行方式。
- [x] 有共享 CMake preset：`OCCTDebug-Debug`。
- [x] 有默认配置：`config/workbench.default.yaml`。
- [x] 有本地配置模板：`config/workbench.local.example.yaml`。
- [x] 本地配置 `config/workbench.local.yaml` 被 gitignore。

## 3. UI 骨架

- [x] 顶部工具栏包含：问题录入、复现生成、源码分析、补丁方案、回归验证、知识归档。
- [x] 左侧包含案例列表、流程状态、关键输入。
- [x] 中央包含源码、复现脚本、几何视图、差异对比、环境信息标签页。
- [x] 右侧包含诊断结论、候选补丁、验证结果、相似案例。
- [x] 底部包含 DRAW 控制台、PowerShell/CMake、testgrid 结果。
- [x] 深色主题统一，控件层次清晰。
- [x] `WorkspaceLayout` 已支持主 splitter 尺寸、中心 tab、底部 tab 和底部高度保存/恢复。
- [x] 主要面板完成当前阶段可维护拆分；左侧 `CasePanel`、中央 `SourcePanel` / `EvidencePanel`、右侧 `VerificationPanel` 已独立。
- [x] Evidence 追加同步已抽到 `EvidenceCoordinator`；testgrid 表格刷新已抽到 `TestgridTablePresenter`。
- [x] testdiff artifact index/analysis 差异表行模型已抽到 `DiffArtifactsPresenter`。
- [x] Diff Compare tab 已拆为 `DiffPanel`，窗口层只负责读取 artifacts 和安全打开/预览。
- [x] 二阶段 final result 状态同步、verification items、Evidence 登记和报告触发意图已抽到 `TwoStageFinalResultCoordinator`。
- [x] 二阶段 final result 输入组装已抽到 `TwoStageFinalResultBuilder`。
- [x] 二阶段 final result JSON 写入已抽到 `TwoStageFinalResultWriter`。
- [x] 二阶段 final 后 UI 控件刷新与保存/报告触发动作已抽到 `TwoStageFinalResultUiAdapter`。
- [x] testdiff adapter result 的 UI 数据、manifest、Evidence 登记和报告触发意图已抽到 `TestdiffAdapterResultCoordinator`。
- [x] EvidenceBundle / VerificationReport 实际写出、路径计算和错误汇总已抽到 `ReportRefreshCoordinator`。

## 4. Case 数据模型

- [x] CaseRecord / CaseManifest 支持 JSON 加载基础。
- [x] CaseManifest 支持 JSON 保存基础。
- [x] CaseManifest 支持 `input.files` 输入文件摘要，包含 Case 相对路径、原始文件名、bytes、SHA-256 和导入时间。
- [x] CaseManifest 支持 `repro.status` 复现状态摘要，包含 overall、DRAW、C++、testgrid、更新时间和摘要。
- [x] Crash dump 文件可归档到当前 Case `artifacts/crash/`，并写出不含本机绝对路径的 SHA-256 manifest。
- [x] 有本地 `cases/<case_id>/` workspace 初始化能力。
- [x] EnvironmentSnapshot 已有 JSON 输出脚本基础。
- [x] EnvironmentSnapshot 已能从 UI 触发并写入当前 Case workspace。
- [x] ReproPack 已有脚本级导出基础。
- [x] ReproPack 已有 UI 导出入口并输出到当前 Case artifacts。
- [x] C++ 最小复现模板已可从 UI 生成到当前 Case `repro/cpp_minimal/`，包含 CMake 工程、`main.cpp`、README 和 DRAW 脚本副本。
- [x] DRAW、C++ 复现模板和 testgrid 结果会统一写回 `repro.status`，并同步到验证面板 `repro status` 指标。
- [x] EvidenceBundle 已有面板/模型骨架。
- [x] EvidenceBundle 已能结构化落盘为 `artifacts/evidence_bundle.json`。
- [x] EvidenceBundle CTest `evidence_bundle_smoke` 已覆盖 sample case 生成路径。
- [x] Evidence 面板行激活已能按 `link` 跳转源码、日志 artifact 或几何检查上下文。
- [x] EvidenceRecord 已支持可选 `source_file/source_line`、`log_file/log_line`、`stack_frame`、`geometry_object` 定位字段。
- [x] Evidence `geometry_object` 已能同步到 OCCT Viewer 子形状高亮基础路径。
- [x] OCCT Viewer 鼠标拾取可反向生成 Geometry Evidence，并写入结构化 selection artifact。
- [x] OCCT Viewer 截图可保存为 Case artifact 并登记为 Geometry Evidence。
- [x] EvidenceBundle 已输出 source/log/geometry `location` 字段，并由 smoke 测试覆盖。
- [x] TopologySignature 已支持 before/after signature JSON 匹配，输出 matches、unmatched、counts_delta 和 summary。
- [x] PatchCandidate 已有 diff、风险、影响模块和审查流程骨架。
- [x] PatchCandidate 审查状态可落盘到 Case manifest，并可导出 `report/patch_review.md`。
- [x] VerificationReport 已有 testgrid/testdiff 结果解析骨架。
- [x] VerificationReport 已能生成 `report/verification_report.md` 和 `verification/verification_report.json`。
- [x] VerificationReport CTest `verification_report_smoke` 已覆盖 sample case 生成路径。
- [x] VerificationReport 已包含 patch dry-run gate 与 testgrid/patch 失败详情基础结构。
- [x] VerificationReport 已包含 `patch_review` gate 和 `patch.decision`，可把 PatchCandidate 审查结论纳入 overall gate。
- [x] VerificationReport 已消费 testgrid/testdiff `failure_details`、`timing` 和 `testdiff_artifacts`，Markdown 已包含 Timing Summary 和 testdiff artifact 链接。
- [x] VerificationReport 已消费 `testdiff_artifacts.artifact_index`，并在 JSON/Markdown 中输出 image/property/performance 索引摘要。
- [x] VerificationReport 已消费 `artifacts/patch_generate_result.json`，Markdown 已包含 Patch generation result 链接。
- [ ] 上述模型全部统一落盘到真实 `cases/<case_id>/` workspace；DRAW、环境采集、Repro Pack 和 EvidenceBundle 已完成基础落盘。

## 5. 自动化执行

- [x] 命令执行器已有记录命令、cwd、stdout、stderr、退出码和耗时的基础能力。
- [x] 支持取消运行中任务；`CommandRunner` 有取消结果语义，DRAW/env/testgrid/testdiff/two-stage/patch UI 已有最小取消入口。
- [x] 支持命令级超时；`CommandRunner` 有 `timedOut` / `timeoutMs` 结果语义，DRAW/env/testgrid/testdiff/two-stage/patch artifact 会记录超时状态。
- [x] 命令输出已有接入底部控制台的基础。
- [ ] 命令执行默认限制在 Case workspace。
- [x] 失败时有明确错误提示的基础。
- [x] DRAW 与 patch 运行证据已能记录可跳转日志行。
- [x] PatchCandidate `git diff --binary HEAD` 生成已使用异步 `CommandRunner`，并记录 stdout/stderr/result artifact。

## 6. OCCT 集成

- [x] 能识别仓库内 OCCT 安装路径。
- [x] 能识别 DRAWEXE。
- [x] 能通过 CTest 运行简单 DRAW 脚本。
- [x] 能通过 CTest 运行最小 `checkshape` smoke。
- [x] 能解析基本 checkshape 输出。
- [x] 能保存 DRAW 运行日志。
- [x] UI DRAW 运行结果能写入当前 Case 的 `logs/` 和 `artifacts/`。
- [x] UI DRAW 运行结果能生成 `draw_log_analysis.json` 和 Evidence 摘要。
- [x] OCCT Viewer 可按 `V/E/W/F/SHELL/SOLID + 序号` 高亮当前加载 shape 的真实子形状。
- [x] OCCT Viewer 支持鼠标拾取反向同步 Evidence 和截图证据基础路径。
- [x] OCCT Viewer 已能生成基于 `BRepTools::Write(subshape)` SHA-256 的 topology signature map，并作为 Case artifact 登记到 Evidence。
- [x] TopologySignature 已有最小 before/after signature 匹配策略，优先 exact hash，再按 object_id/orientation/children/index 近似评分。
- [x] TopologySignature 已补充局部几何和子拓扑统计特征，匹配策略会参考 measure、bbox center/diagonal 和 subshape counts。
- [x] before/after topology match artifact 已接入几何差异 tab、EvidenceBundle 和 VerificationReport。
- [x] 几何差异 tab 已支持选择 before/after topology signature JSON 并生成 `artifacts/topology_compare.json`。
- [x] testgrid artifact 路径、目录创建、命令日志、phase summary 和 JSON result 写入已抽到 `TestgridArtifactService`。
- [x] 单阶段 testgrid result 解析、verification items、diff summary、failure details、timing 和 `artifacts/testgrid_result.json` 组装已抽到 `TestgridResultWriter`。
- [x] 二阶段 before/after phase result 的日志写入、summary 解析/写入、failure/timing 构造和 JSON artifact 写入已抽到 `TwoStagePhaseResultWriter`。
- [x] 外部 testdiff runner 输出目录可通过 `TestdiffRunnerAdapter` 归一化导入到 `artifacts/testdiff/{before,after,diff}`。
- [x] testdiff 命令规划已抽到 `TestdiffCommandPlanner`，覆盖 executable 校验、输出目录创建、工作目录 fallback、占位符替换和 `CommandRequest` 组装。
- [x] `TestdiffRunnerAdapter` 已接入底部 testgrid 面板 `Run testdiff`，支持配置命令、输出目录、日志和 adapter manifest 落盘。
- [x] testdiff artifact 已生成 `artifact_index`，可按 normalized key 汇总 image/property/performance 工件的 before/after/diff 配对状态、计数和策略说明。
- [x] testdiff artifact 已生成 `artifact_analysis`，可对 runner 已提供的 image/property/performance 工件做可用性、属性 JSON 和性能文本指标的轻量解析。
- [x] testdiff 真实生成器边界已固化为 `TestdiffGenerationPolicy`，`artifact_analysis.generation_policy` 会记录候选输入、禁用状态、阻塞原因和后续契约，当前不生成伪 artifact。
- [x] testdiff 真实生成器 opt-in 契约已固化为 `TestdiffGenerationContract`，定义 manifest 字段、Case 相对输出根、sidecar 命名、三类生成器输出模式和隐私边界。
- [x] testdiff adapter runner 日志、summary、adapter result/manifest 和兼容 `testgrid_result.json` 写入已抽到 `TestdiffAdapterResultWriter`。
- [ ] OCCT Viewer 尚需更强的跨拓扑重建永久命名，当前 compare artifact 生成仍基于已落盘 topology signature 和启发式局部几何匹配。
- [x] EvidenceBundle/VerificationReport 实际写出已抽成 `ReportRefreshCoordinator`，并由 `report_refresh_coordinator_smoke` 覆盖。
- [ ] `WorkbenchWindow` 中 Case 保存、命令编排和 UI 刷新职责仍需继续收束。
- [ ] testdiff 仍未实现图片 diff/属性结构 diff/性能趋势生成算法；当前只导入、索引、轻量解析 runner 已生成的 before/after/diff 工件，并输出 boundary-only 生成策略与 opt-in 输出契约。
- [x] 已有 testgrid/testdiff 结果解析骨架。
- [x] testgrid/testdiff 最小 runner 已接入 UI 和 Case：支持 `draw_smoke` 前置门禁、`testgrid_plan` 配置和结果落盘。
- [x] testgrid before/after 最小对比已支持可选 `verification/testgrid_before.txt` / `verification/testgrid_after.txt`，并写入 VerificationReport 与 EvidenceBundle。
- [x] testgrid/testdiff 已有 UI 二阶段验证入口，可编排 before gate/command、patch apply、after gate/command 和 patch undo，并写入 two-stage artifact。
- [x] 二阶段验证状态机已抽成 `VerificationWorkflow` 服务，并由 `verification_workflow_smoke` 覆盖关键状态转移。
- [x] 二阶段验证 result JSON 组装已抽成 `TwoStageVerificationResultWriter`，并由 `two_stage_verification_result_writer_smoke` 覆盖。
- [x] 二阶段 phase result 写入已抽成 `TwoStagePhaseResultWriter`，并由 `two_stage_phase_result_writer_smoke` 覆盖。
- [x] 二阶段 final result 写入已抽成 `TwoStageFinalResultWriter`，并由 `two_stage_verification_result_writer_smoke` 覆盖实际 artifact 落盘。
- [x] 二阶段 final 输入组装已抽成 `TwoStageFinalResultBuilder`，并由 `two_stage_final_result_builder_smoke` 覆盖 writer input、回归对比、失败明细、耗时和 testdiff artifact 扫描。
- [x] 二阶段 final 后 UI 控件刷新与保存/报告触发动作已抽成 `TwoStageFinalResultUiAdapter`，并由 `workbench_presenter_smoke` 覆盖。
- [x] topology signature 已由 `topology_signature_smoke` 覆盖基础结构和对象签名。
- [x] testgrid/testdiff artifact 已输出结构化失败明细、耗时和 testdiff summary/stdout/stderr 工件指针，并由 `verification_result_parser_smoke`、`verification_report_smoke`、`evidence_bundle_smoke` 覆盖。
- [x] testdiff artifact 已支持扫描真实 before/after/diff 目录，并归档图片、属性、性能、日志和文本等细粒度工件清单。
- [x] testdiff artifact 已支持 image/property/performance 工件索引，输出 `artifact_index` 供报告和 UI 后续消费。
- [x] testdiff artifact 已支持 `artifact_analysis`，输出图片 diff 来源、属性 JSON 摘要和性能文本指标，供报告和后续 UI 消费。
- [x] testdiff artifact 已支持 `generation_policy.contract`，明确真实图片像素 diff、属性结构 diff、性能趋势 diff 只能通过 opt-in 配置、Case 相对输出和专门 smoke 覆盖后才能启用。
- [x] VerificationReport 与 EvidenceBundle 已展示/归档 `artifact_index_summary`，覆盖 kind 组数和配对状态计数。
- [x] VerificationReport 与 EvidenceBundle 已展示/归档 `artifact_analysis` 摘要。
- [x] 差异对比 tab 已展示 `artifact_index` 与 `artifact_analysis` 的专门表格视图。
- [x] 差异对比 tab 已支持 artifact kind/status 过滤，并可安全打开当前 Case workspace 内的 testdiff artifact。
- [ ] testgrid/testdiff 完整 runner 仍需接入图片像素 diff、属性结构 diff、性能趋势 diff 的真实生成器或更完整的外部适配器。

## 7. 报告与归档

- [x] 已有 Markdown 报告生成骨架。
- [x] 报告可从当前 Case 数据生成到 `case/report/repro_report.md`。
- [x] 补丁审查报告可从当前 Case 数据生成到 `case/report/patch_review.md`。
- [x] 补丁审查报告会链接当前 Case 的 VerificationReport。
- [x] 验证报告可从当前 Case 数据生成到 `case/report/verification_report.md` 和 `case/verification/verification_report.json`。
- [x] EvidenceBundle 已归档 `verification_failures`、`verification_timing` 和 `testdiff_artifacts`。
- [x] VerificationReport 与 EvidenceBundle 已保留 `testdiff_artifacts.artifact_files`、`directories`、`artifact_counts` 和 `truncated`。
- [x] VerificationReport 与 EvidenceBundle 已专门展示/归档 `testdiff_artifacts.artifact_index` 摘要。
- [x] VerificationReport 与 EvidenceBundle 已专门展示/归档 `testdiff_artifacts.artifact_analysis` 摘要。
- [x] 差异对比 UI 已接入 `testdiff_artifacts.artifact_index` 与 `artifact_analysis` 表格摘要，并支持 artifact 打开/过滤。
- [x] DiffPanel 已补充图片内嵌预览、路径复制和更细的工件搜索。
- [ ] 报告包含完整真实 Case 的环境、复现、证据、结论、验证和风险。
- [x] 报告已有最小本机绝对路径脱敏。
- [ ] 私有路径和敏感文件名有完整、可配置的脱敏策略。
- [ ] Case artifacts 目录结构稳定。

## 8. Patch 审查与应用

- [x] Patch Review 状态可写回 Case manifest。
- [x] Patch Review 面板可导出审查报告。
- [x] `patch.worktree_root` 可选配置已加入默认/本地配置文件。
- [x] UI 可触发 `git apply` / `git apply -R` 的最小 Apply/Undo 流程。
- [x] Patch Apply/Undo 结果会写入 Case logs/artifacts/Evidence/verification items。
- [x] Patch Apply/Undo 前已执行 `git apply --check` / `git apply -R --check` dry-run 预检，失败时不修改 worktree。
- [x] Patch dry-run/apply 失败日志可通过 Evidence 定位到对应日志行。
- [x] Patch dry-run 失败会输出结构化 conflict hints，供后续交互式冲突定位使用。
- [x] Patch 审查状态和审查项已进入 EvidenceBundle 与 VerificationReport。
- [x] PatchCandidate diff 可从 `.patch/.diff` 导入、保存为 Case artifact、导出为 `.patch`。
- [x] PatchCandidate 可从配置的 `patch.worktree_root` 异步执行 `git diff --binary HEAD` 生成候选 diff。
- [x] PatchCandidate 生成结果会写入 `logs/patch_generate.stdout.log`、`logs/patch_generate.stderr.log` 和 `artifacts/patch_generate_result.json`。
- [x] Patch candidate manifest 已记录相对路径、bytes、sha256、review/signoff 状态和 worktree 配置状态。
- [x] Patch signoff 已接入 VerificationReport 门禁，并写入 EvidenceBundle 与 VerificationReport。
- [x] 支持基于现有候选 diff 的自动 before/after 验证编排最小闭环。
- [ ] 支持自动源码修复生成、交互式冲突修复和自动提交/合并。

## 9. 质量门禁

每次合并前至少确认：

- [ ] 修改范围可解释。
- [ ] 构建或最小验证已运行。
- [ ] 没有提交临时文件、CAD 私有数据、dump、PDB、大型 build 产物。
- [ ] 没有无关格式化大改。
- [ ] UI 代码与业务逻辑没有严重耦合。
- [ ] README 或 doc 已同步更新。
