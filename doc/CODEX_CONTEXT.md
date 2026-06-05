# OCCTDebug Codex 项目上下文摘要

更新时间：2026-06-05

## 1. 一句话目标

OCCTDebug 是 Windows 本地的“OCCT 内核专家工作台”：把用户的 OCCT 问题转化为可复现 Case、环境快照、DRAW/C++ 复现、证据链、源码定位、候选补丁、回归验证和知识沉淀。

项目不做聊天式问答工具，而做可审查、可运行、可归档的工程工作台。

## 2. 当前实现基线

当前仓库已经从旧代码重搭为新的 Qt Widgets 工作台，并完成以下基础能力：

- Qt 主窗口骨架：顶部工具栏、左侧 Case/流程/输入区、中央源码/复现/几何/差异/环境标签页、右侧诊断/补丁/验证/相似案例、底部 DRAW/PowerShell-CMake/testgrid 控制台。
- Case 数据模型：`CaseManifest` 已支持 JSON 加载和 mock/sample 数据驱动 UI。
- Workflow/Layout：`CaseManifest` 已包含 `WorkflowState` 与 `WorkspaceLayout` 基础模型，兼容旧 `workflow` 字段，并能读写 `workflow_state` / `workspace_layout`；主 splitter 尺寸、中心 tab 和底部 tab 已支持保存/恢复。
- 配置与上下文：已有 `config/workbench.default.yaml`、`config/workbench.local.example.yaml`、`ConfigService` 和 `AppContext`。
- 构建/运行脚本：已有 `scripts/build.ps1` 封装 configure/build/CTest，已有 `scripts/run.ps1` 定位并启动当前构建出的 `OCCTDebug.exe`。
- Case workspace：已有 `CaseWorkspaceService`，应用启动时可从 `sample_cases` 初始化本地 ignored 的 `cases/<case_id>/`。
- 输入文件：`CaseManifest` 已支持 `input.files`，记录 Case 相对路径、原始文件名、bytes、SHA-256 和导入时间；几何模型导入成功后会自动登记输入文件摘要，repro report 会展示该摘要。
- Crash dump 归档：Evidence 面板已有 `Archive dump` 入口，可将 `.dmp/.mdmp/.dump` 复制到当前 Case `artifacts/crash/`，计算 SHA-256，写出不含本机绝对路径的 manifest，并登记为 Evidence。
- Case UI：左侧 Case 面板已拆为 `workbench/CasePanel.*`，支持从 `cases/` 扫描列表、新建 Case、打开包含 `case.json` 的 Case 目录、保存当前 Case、双击切换 Case。
- Workbench UI 拆分：左侧 Case 面板、中央源码面板、中央证据面板和右侧验证结果面板已分别拆为 `workbench/CasePanel.*`、`workbench/SourcePanel.*`、`workbench/EvidencePanel.*`、`workbench/VerificationPanel.*`；主窗口继续负责 Case workspace 编排。
- 环境采集：`scripts/verify_env.ps1` 可输出 Windows、VS、CMake、Qt、OCCT、Tcl/Tk、FreeType、DRAWEXE 与 DRAW CTest 结果。
- 环境采集 UI 闭环：环境信息 tab 可触发 `verify_env.ps1`，并将 `env_snapshot.json`、stdout/stderr、result JSON 写入当前 Case workspace。
- 命令执行：`CommandRunner` 已提供统一外部命令执行基础，`CommandResult` 可标记 `canceled` / `timedOut`，并记录 `timeoutMs`；DRAW、环境采集、testgrid/testdiff/二阶段验证和 patch 命令已有最小取消入口与超时配置，UI 日志和结果 artifact 会区分 passed/failed/canceled/timed_out。
- DRAW 基础闭环：UI 已有 DRAW 脚本编辑/运行入口；CTest 已有 `draw_smoke` 和 `draw_checkshape_smoke`。
- DRAW Case 落盘：UI 运行 DRAW 后会把 `repro.tcl`、stdout/stderr、`draw_result.json`、`draw_log_analysis.json` 和 Evidence 写入当前 Case workspace。
- C++ 复现模板：复现脚本 tab 已提供 `C++ Repro` 入口，可在当前 Case 的 `repro/cpp_minimal/` 下生成最小 CMake 工程、`main.cpp`、README 和当前 DRAW 脚本副本；模板只给出可编译的 OCCT 复现工程骨架，不自动把 Tcl 语义转换为 C++ 问题代码。
- 复现状态：`CaseManifest` 已有 `repro.status` 结构化字段，DRAW、C++ 复现模板和 testgrid 结果会统一写回 draw/cpp/testgrid/overall 状态，并同步到验证面板的 `repro status` 指标。
- Repro Pack：验证结果面板可导出当前 Case Repro Pack 到 `artifacts/repro_pack/`。
- Markdown 报告：验证结果面板可生成当前 Case 报告，报告生成器已有 Evidence artifact 链接有效性检查和本机绝对路径脱敏。
- testgrid/testdiff：底部 testgrid 面板可运行最小 `draw_smoke` 门禁，并支持 Case/配置中的 `testgrid_plan`（root/executable/arguments/group/grid/case/testdiff_executable/testdiff_arguments/testdiff_output_root）。未配置真实 testgrid executable 时只解析当前 Case `verification/testgrid_summary.txt` / `testdiff_summary.txt`；配置后门禁通过再运行指定 testgrid 命令，结果写入 `artifacts/testgrid_result.json`。`Run testdiff` 入口会先运行 `draw_smoke` 门禁，再通过 `core/verify/TestdiffCommandPlanner.*` 生成配置 testdiff 命令，执行后把 runner 输出中的 before/after/diff 目录通过 `TestdiffRunnerAdapter` 导入到 `artifacts/testdiff/{before,after,diff}`。`core/verify/TestdiffAdapterResultWriter.*` 负责写出 `logs/testdiff_runner.*`、`verification/testdiff_summary.txt`、`artifacts/testdiff_adapter_result.json`、`artifacts/testdiff_adapter_manifest.json` 和兼容的 `artifacts/testgrid_result.json`；`workbench/TestdiffAdapterResultCoordinator.*` 负责同步 UI 数据、Case manifest、Evidence 和报告刷新触发意图。若 Case 中存在 `verification/testgrid_before.txt` / `verification/testgrid_after.txt`，或 UI 运行结果写入 `before_after`，会生成 pass/fail delta 与回归状态。当前还提供“二阶段验证”入口，可按 before gate/command、patch dry-run/apply、after gate/command、patch undo 的顺序编排最小 before/after 验证，并写入 `artifacts/testgrid_two_stage_result.json` 与兼容的 `artifacts/testgrid_result.json`。单阶段与二阶段 artifact 现在统一包含 `failure_details`、`timing`、`testdiff_artifacts`，用于归档真实 testgrid/testdiff 失败明细、耗时节点和 testdiff summary/日志指针。`core/verify/TestdiffArtifactScanner.*` 会扫描 Case workspace 内约定的 before/after/diff testdiff 目录，并把图片、属性、性能、日志和文本工件归档到 `testdiff_artifacts.artifact_files`；`core/verify/TestdiffArtifactIndex.*` 会进一步把 image/property/performance 工件按 normalized key 归并成 `artifact_index`，记录 before/after/diff 配对状态、计数和策略说明；`core/verify/TestdiffArtifactAnalysis.*` 会对已有 runner 工件做轻量分析，记录图片 before/after/diff 可用性，解析属性 JSON 的类型和顶层 key，提取性能文本中的简单数值指标；`core/verify/TestdiffGenerationContract.*` 定义真实生成器未来 opt-in 的 manifest 字段、Case 相对输出根、sidecar 命名和隐私边界；`core/verify/TestdiffGenerationPolicy.*` 会在 `artifact_analysis.generation_policy` 中记录图片像素 diff、属性结构 diff 和性能趋势 diff 的候选输入、阻塞原因，并携带 `generation_policy.contract`。VerificationReport 与 EvidenceBundle 已输出 `artifact_index` / `artifact_analysis` 明细和摘要，Markdown 验证报告会展示 Testdiff Artifact Index 与 Testdiff Artifact Analysis 小节；差异对比 tab 已拆为 `workbench/DiffPanel.*`，由 panel 持有摘要 label、kind/status 过滤、搜索框、artifact index 表、artifact analysis 表、路径复制和图片预览区域，并复用 `DiffArtifactsPresenter` 生成表格行。`WorkbenchWindow` 只负责读取最新 `testgrid_result.json`、把 `testdiff_artifacts` 喂给 panel，以及执行 Case workspace 内的安全打开/预览逻辑；文本、日志和 JSON 在底部控制台预览，图片等二进制工件既可内嵌预览，也可走系统关联程序。当前只消费已有 runner 工件，生成策略为 boundary-only：真实生成器必须通过 `verification.testdiff_generation.enabled_generators` 显式 opt-in 后才能进入实现阶段；现阶段不启用图片像素 diff 生成、不计算属性结构 diff、不建立性能趋势基线，也不写出伪 artifact。二阶段状态机已抽到 `core/verify/VerificationWorkflow.*`，二阶段 phase/final result JSON 组装已抽到 `core/verify/TwoStageVerificationResultWriter.*`；二阶段 phase result 的日志、summary、failure/timing 和 `artifacts/testgrid_<phase>_result.json` 写入已抽到 `core/verify/TwoStagePhaseResultWriter.*`；二阶段 final 输入组装已抽到 `core/verify/TwoStageFinalResultBuilder.*`，final artifact 写入已抽到 `core/verify/TwoStageFinalResultWriter.*`；EvidenceBundle/VerificationReport 实际写出已抽到 `workbench/ReportRefreshCoordinator.*`。`WorkbenchWindow` 仍保留命令启动、Case 保存和 UI 刷新编排。
- testdiff 生成器 N73：`CaseManifest` 已可保存 `verification.testdiff_generation`，包含 `enabled_generators`、图片/属性容差、性能回归阈值和失败报告路径；`TestdiffGenerationContract` 已输出默认配置与失败报告 schema；`TestdiffGenerationPolicy` 可识别显式 opt-in 并输出每个生成器的有效配置、阻塞原因和 `failure_report` 状态，但仍保持 `enabled=false`、`generation_performed=false`，不写伪 artifact。新增 `testdiff_generation_failure_report_smoke` 覆盖该边界。
- Case 保存同步 N74：`workbench/CaseManifestSync.*` 已把保存 Case 时的可变字段回填从 `WorkbenchWindow` 抽成纯 helper；`WorkbenchMockData` 已携带 `testdiffGenerationConfig`，从 Case 加载时保留 `verification.testdiff_generation` 并在保存同步时写回 manifest。`workbench_presenter_smoke` 覆盖 repro、testdiff generation、patch、testgrid 和 workflow fallback 同步。
- Source/Diagnosis：源码 tab 可本地关键词搜索并跳转文件行；相似案例面板可按关键词或当前诊断重排；诊断面板可导出 `report/diagnosis_report.md` 并登记为 Evidence。
- Patch Review：候选补丁审查状态已接入 Case manifest，可在 UI 中标记送审/通过/退回，并导出 `report/patch_review.md`；候选补丁 diff 可手工编辑、从 `.patch/.diff` 导入、导出为 `.patch`，也可基于可选 `patch.worktree_root` 异步执行 `git diff --binary HEAD` 生成。该生成流程通过 `CommandRunner` 记录命令、cwd、stdout/stderr、退出码和耗时，写入 `logs/patch_generate.stdout.log`、`logs/patch_generate.stderr.log` 与 `artifacts/patch_generate_result.json`；成功且有 diff 时再保存到 `artifacts/candidate_patch.diff` 和 `candidate_patch_manifest.json`。Patch Review 面板已有 Apply/Undo 最小入口，通过可选 `patch.worktree_root` 在目标 worktree 中执行 `git apply --check` / `git apply -R --check` dry-run，通过后再执行 `git apply` / `git apply -R`，结果写入 Case logs/artifacts/Evidence。审查报告会链接最新 VerificationReport。Patch signoff 会读取最新 VerificationReport，只有 overall passed 且 patch review gate 可接受时才写入 `signed off`，否则写入 `blocked`。当前仍不自动生成源码修复，也不自动提交或合并。
- EvidenceBundle：`core/evidence/EvidenceBundleWriter.*` 会把当前 Case evidence records、source/log/geometry/artifact 分类、可选定位字段、几何检查、验证指标、before/after 验证对比、testgrid/testdiff 失败明细、验证耗时、testdiff 工件、`artifact_index` / `artifact_analysis` 摘要、诊断和 patch 状态写入 `artifacts/evidence_bundle.json`；patch 生成状态、patch dry-run、patch 审查状态和审查项也会进入 patch 摘要。Markdown 报告会在文件存在时链接该结构化证据包。`evidence_bundle_smoke` CTest 覆盖 sample case 生成路径、source/log/geometry location、verification comparison、N35 testdiff artifacts、N36 patch generation、N52/N54 artifact summary 和 patch review 输出，并检查不写入本机绝对路径。
- VerificationReport：`core/verify/VerificationReportWriter.*` 会把当前 Case 的 DRAW gate、testgrid/testdiff、before/after 对比、testgrid/testdiff 结构化失败明细、验证耗时、testdiff 工件、testdiff artifact index/analysis 摘要、patch 生成结果、patch dry-run/apply 状态、patch 审查结论、patch signoff、patch conflict hints、failure details、EvidenceBundle 链接和 overall gate 写入 `report/verification_report.md` 与 `verification/verification_report.json`；Markdown 报告包含 Timing Summary、Testdiff Artifact Index、Testdiff Artifact Analysis、testdiff artifact 链接和 patch generation result 链接。`gate.patch_review` 会把 PatchCandidate 审查状态纳入验证门禁，`gate.patch_signoff` 会记录验证签核状态。审查通过但验证仍失败时保持 `blocked`，审查拒绝或签核 blocked 时进入 `failed`。主窗口保存 Case、导出 patch review 和执行 patch signoff 后会同步生成该验证报告，`repro_report.md` 会链接该验证报告。`verification_report_smoke` CTest 覆盖 sample case 生成路径、N35/N36/N52/N54 字段，并检查不写入本机绝对路径。
- Evidence UI 联动：Evidence 面板支持行激活信号，主窗口优先按 `source_file/source_line`、`log_file/log_line`、`stack_frame`、`geometry_object` 结构化字段跳转，也兼容 `file:line` 与 `logs/x.log:line` 链接约定；跳转目标仍限制在仓库源码或当前 Case 相对 artifact 内。当前可定位源码行、日志行、几何检查项，并可把 `geometry_object` 同步到 OCCT Viewer 子形状高亮。
- Workbench presenter / verify 拆分：`workbench/EvidenceCoordinator.*` 已负责 EvidenceRecord 追加到 `WorkbenchMockData` 与 `CaseManifest` 的同步，`workbench/TestgridTablePresenter.*` 已负责把 testgrid rows 映射为表格单元并刷新 `QTableWidget`，`workbench/DiffArtifactsPresenter.*` 已负责把 testdiff artifact index/analysis 映射为差异表格单元，`workbench/DiffPanel.*` 已负责差异页控件组装、过滤、双击跳转信号和当前 artifacts 状态。`workbench/TwoStageFinalResultCoordinator.*` 已负责二阶段 final result 的 WorkbenchMockData/CaseManifest 状态同步、verification items、Evidence 登记和 EvidenceBundle/VerificationReport 触发意图；`workbench/TwoStageFinalResultUiAdapter.*` 已负责二阶段 final 后 diff label、testgrid 表、VerificationPanel 和 EvidencePanel 的控件刷新，并返回保存/报告动作。`workbench/ReportRefreshCoordinator.*` 已负责 EvidenceBundle/VerificationReport 的实际写出、路径计算和错误汇总。`core/verify/TestgridResultWriter.*` 已负责单阶段 testgrid result 的解析、verification items、diff summary、failure details、timing 和 `artifacts/testgrid_result.json` 写入。`core/verify/TestdiffAdapterResultWriter.*` 与 `workbench/TestdiffAdapterResultCoordinator.*` 已负责 testdiff adapter result 写入和 UI/manifest/Evidence/报告触发同步。`core/verify/TwoStagePhaseResultWriter.*` 已负责二阶段 phase result 解析和文件写入，`core/verify/TwoStageFinalResultBuilder.*` 已负责二阶段 final writer/coordinator 输入组装，`core/verify/TwoStageFinalResultWriter.*` 已负责二阶段 final result 写入。当前 `WorkbenchWindow` 仍保留 Case 保存、命令启动和 UI 刷新编排。
- DRAW 运行辅助：已支持 DRAW 日志解析、临时 repro Tcl 注册为 CTest、失败 case 导出 Repro Pack。
- OCCT Viewer：已有基础 Qt/OCCT Viewer 集成入口；几何 tab 可导入 `.brep` / `.brp` / `.rle` / `.step` / `.stp` / `.iges` / `.igs` 到当前 Case `input/` 并加载显示。Viewer 采用懒加载，避免主窗口启动阶段提前初始化 native OpenGL/WNT viewport。加载 demo 或模型后，Viewer 的 `topologySummary()` 会同步为几何检查表中的 `Topology stats` 行并写回 Case manifest。Viewer 当前可按 `V/E/W/F/SHELL/SOLID/COMPSOLID/COMPOUND/SHAPE + 序号` 解析 `geometry_object`，通过 `TopExp_Explorer` 找到当前 shape 的真实子形状并用 `AIS_Shape` 着色高亮；鼠标左键拾取会反向生成 Geometry Evidence 并写入 `artifacts/geometry_selection_*.json`。几何 tab 可保存当前 Viewer PNG 截图到 `artifacts/geometry_screenshot_*.png` 并登记为 Evidence。`core/geometry/TopologySignature.*` 会基于 `BRepTools::Write(subshape)` SHA-256 生成 topology signature map，模型加载、拾取、截图或手动“保存映射”时写入 `artifacts/topology_signature_*.json`；签名记录已包含子拓扑计数、bounding box、bbox center/diagonal、线/面/体 measure 和 center of mass。`compare()` 对 before/after signature JSON 做同类型精确 hash 优先、object_id/orientation/children/index 加局部几何与子拓扑计数的近似评分，并输出可审查策略提示。`core/geometry/TopologyCompareArtifact.*` 负责读取 `artifacts/topology_compare.json`、从 before/after signature 内存生成 compare 对象，或把用户选择的 before/after signature 生成并落盘到当前 Case 的 compare artifact；几何差异 tab、EvidenceBundle 和 VerificationReport 已消费 `geometry_diff`，生成入口会同步登记 Geometry Evidence。当前签名和匹配策略能支撑几何差异审查，但仍不是跨所有拓扑重建场景的完整永久命名。
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
2. 采集环境并保存到 Case artifacts。（基础已完成）
3. 在 UI 中编辑并运行 DRAW 脚本。（基础已完成）
4. 解析 DRAW 日志和 checkshape 结果。（基础已完成）
5. 把日志、环境、复现脚本组成 Evidence。（基础已完成）
6. 生成 Markdown 报告。（基础已完成，已有最小路径脱敏）
7. 导出 Repro Pack。（基础已完成）

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

1. 完善命令任务队列/状态面板，把当前最小状态日志升级为可浏览的任务历史。
2. 为真实 testdiff 生成器输出 sidecar 与 failure report writer 后，再启用具体生成算法。
3. 继续收束 WorkbenchWindow 中剩余命令编排和 UI 刷新职责。
