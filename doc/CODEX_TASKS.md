# OCCTDebug Codex 任务模板

更新时间：2026-06-05

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
- N1：配置系统与 AppContext 基础，包含共享 preset、默认配置、本地配置模板、ConfigService、AppContext。
- N2：Case workspace 基础，包含 `CaseWorkspaceService`、标准目录、sample case 初始化、manifest 保存/加载。
- N3：UI 初始数据已从 sample Case workspace 加载；完整多 Case 切换仍待实现。
- N4：DRAW UI 运行结果已落盘到当前 Case，并生成基础 DRAW Evidence。
- N5：DRAW success token、错误行、checkshape 状态已进入 Evidence，并生成 `draw_log_analysis.json`。
- N6：环境信息 tab 已可触发 `verify_env.ps1`，并保存 `env_snapshot.json`、日志和 result JSON 到当前 Case。
- N7：当前 Case Markdown 报告可导出，已有最小本机绝对路径脱敏。
- N8：当前 Case Repro Pack 可从 UI 导出到 `artifacts/repro_pack/`。
- N10：Case 面板已支持扫描 `cases/`、新建、打开、保存和双击切换 Case。
- N9：`WorkflowState` / `WorkspaceLayout` 基础模型已接入 `CaseManifest`，sample case 已包含 `workflow_state` / `workspace_layout`。
- N11：Markdown 报告已检查 Evidence artifact 相对链接，缺失/越界/绝对路径会在报告中标记并脱敏。
- N12/N15：OCCT Viewer 可导入 `.brep` / `.brp` / `.rle` / `.step` / `.stp` / `.iges` / `.igs` 到当前 Case `input/` 并加载显示；Viewer 懒加载，主窗口启动不提前初始化 native viewport。
- N13/N16：底部 testgrid 面板可运行 `draw_smoke` 门禁；`testgrid_plan` 支持 root/executable/arguments/group/grid/case 配置，未配置 executable 时解析当前 Case testgrid/testdiff summary，配置后门禁通过再运行指定命令，并写入 `artifacts/testgrid_result.json`。
- N14：源码 tab 可本地搜索并跳转文件行；相似案例可按关键词/诊断重排；诊断报告可导出到 `report/diagnosis_report.md`。
- N17：候选补丁审查状态可写回 Case manifest；UI 支持送审/通过/退回并导出 `report/patch_review.md`，真实 patch 应用/撤销仍保留为后续任务。
- N18：左侧 Case 面板已拆为 `workbench/CasePanel.*`，主窗口通过信号处理新建/打开/保存/刷新/切换，Case 业务逻辑仍留在 `WorkbenchWindow` 侧。
- N19：`WorkspaceLayout` 已支持主 splitter 左/中/右宽度、中心 tab、底部 tab 和底部高度的稳定 ID 保存/恢复。
- N20：`CasePanel`、`VerificationPanel`、`EvidencePanel`、`SourcePanel` 已完成当前阶段拆分；主窗口保留 Case workspace、Runner、索引与报告编排。
- N21：Patch Review 面板已有 Apply/Undo 最小入口；`patch.worktree_root` 可选配置，执行 `git apply` / `git apply -R`，结果写入 Case logs/artifacts/Evidence/verification items。真实补丁生成、冲突交互处理和提交仍待后续完善。
- N22：`core/evidence/EvidenceBundleWriter.*` 已生成 `artifacts/evidence_bundle.json`，包含 Evidence records、source/log/geometry/artifact 分类、几何检查、验证指标、诊断和 patch 状态；新增 `evidence_bundle_smoke` CTest 验证 sample case 可生成结构化证据包且不写入本机绝对路径。源码/日志/几何 UI 联动仍待后续完善。
- N23：`core/verify/VerificationReportWriter.*` 已生成 `report/verification_report.md` 和 `verification/verification_report.json`，汇总 DRAW gate、testgrid/testdiff、patch apply 状态、EvidenceBundle 链接和 overall gate；主窗口保存 Case 时会同步写出，`repro_report.md` 会链接该验证报告；新增 `verification_report_smoke` CTest 验证 sample case 可生成结构化验证报告且不写入本机绝对路径。
- N24：Evidence 面板行激活已接入工作台跳转：`file:line` 源码引用切到源码 tab 并尝试打开对应文件，日志/普通 artifact 保持 Case 相对路径并显示到底部控制台，Shape/Geometry 证据切到几何 tab 并选中失败/告警检查项。调用栈精确跳转、日志行号定位和真实几何对象高亮仍待后续增强。
- N25：Patch Apply/Undo 前已执行 `git apply --check` / `git apply -R --check` dry-run；失败时不修改 worktree，结果写入 `logs/patch_*_dry_run.*` 与 `artifacts/patch_*_dry_run_result.json`，并同步 Evidence、verification items、EvidenceBundle 和 VerificationReport；VerificationReport 新增 `patch_dry_run` gate 与 `failure_details`，`verification_report_smoke` 已覆盖这些字段。异步 dry-run、交互式冲突处理和 before/after 对比仍待后续增强。
- N26：`EvidenceRecord` 已支持 `source_file/source_line`、`log_file/log_line`、`stack_frame`、`geometry_object` 可选定位字段；Evidence 激活时优先使用结构化字段，支持源码行、日志行、调用栈帧解析和几何对象匹配；DRAW 与 patch 运行证据会记录可跳转日志行；EvidenceBundle 已输出 `location` 对象，`evidence_bundle_smoke` 覆盖这些字段。真实 OCCT shape/edge/face 视图高亮仍待后续 Viewer 层增强。
- N27：新增 testgrid before/after 最小对比模型；当前支持读取 `verification/testgrid_before.txt` / `verification/testgrid_after.txt` 或 UI 运行落盘的 `artifacts/testgrid_result.json.before_after`，生成 fail/pass delta、回归状态和失败详情。VerificationReport 新增 `before_after` gate、JSON 区块和 Markdown 区块；EvidenceBundle 新增 `verification_comparison`；patch dry-run 失败会结构化输出 `conflicts` hints。完整自动两阶段 testgrid 编排、冲突交互修复和真实 patch 生成仍待后续增强。
- N28：OCCT Viewer 已支持按 `geometry_object` 高亮当前加载 shape 的子形状：支持 `V/E/W/F/SHELL/SOLID + 序号`，通过 `TopExp_Explorer` 查找真实 `TopoDS_Shape` 并用单独 `AIS_Shape` 着色显示。Evidence 激活会同步几何 tab、几何检查表和 Viewer 高亮状态；找不到对象时会显示明确失败原因。稳定命名拓扑、鼠标拾取反向同步和截图证据仍待后续增强。
- N29：VerificationReport 已绑定 PatchCandidate 审查结论；`verification_report.json` 新增 `gate.patch_review` 和 `patch.decision`，Markdown 报告新增审查门禁、建议和审查项表格。审查通过但验证仍失败时会进入 `blocked`，并把 `candidate_patch` 写入 `failure_details`；`patch_review.md` 会链接最新验证报告；EvidenceBundle 已输出 patch 审查状态和审查项。自动二阶段 testgrid/testdiff 的最小编排已由 N30 补齐。
- N30：底部 testgrid 面板新增“二阶段验证”入口；工作台可按 before gate/command -> patch dry-run/apply -> after gate/command -> patch undo 的顺序编排最小两阶段验证。每个阶段会写入独立日志、`verification/testgrid_before.txt` / `verification/testgrid_after.txt`、`artifacts/testgrid_before_result.json` / `testgrid_after_result.json`，最终写入 `artifacts/testgrid_two_stage_result.json`，并同步 `artifacts/testgrid_result.json` 给 VerificationReport/EvidenceBundle 读取。未配置真实 testgrid executable 时仍只使用 DRAW gate 和本地 summary 文件；真实大型回归、失败明细和交互式冲突修复仍待后续增强。
- N31：OCCT Viewer 已支持鼠标左键拾取子形状并反向生成 Geometry Evidence；拾取对象会规范为 `V/E/W/F/SHELL/SOLID/COMPSOLID/COMPOUND/SHAPE + 序号`，并写入 `artifacts/geometry_selection_*.json`。几何 tab 新增“保存截图”按钮，可生成 `artifacts/geometry_screenshot_*.png` 并登记为截图 Evidence；EvidenceBundle 会通过既有 artifact/location 字段归档这些证据。当前稳定标识仍基于当前 shape 的 `TopExp_Explorer` 遍历序号，尚不是跨模型修复/拓扑重建的永久命名。
- N32：候选补丁面板已支持编辑 diff、从配置的 `patch.worktree_root` 执行 `git diff --binary HEAD` 生成候选、导入 `.patch/.diff`、保存到 `artifacts/candidate_patch.diff` 并写出 `candidate_patch_manifest.json`、导出 `.patch` 文件。Patch signoff 会读取最新 `verification/verification_report.json`，只有 overall passed 且 patch review gate 可接受时才写入 `signed off`，否则写入 `blocked`；VerificationReport 新增 `gate.patch_signoff` 和 `patch.signoff_*` 字段，EvidenceBundle 也会归档 signoff 状态。当前仍不自动生成源码修复、不自动提交/合并，也未提供交互式冲突修复 UI。
- N33：二阶段验证状态机已从 `WorkbenchWindow` 抽到 `core/verify/VerificationWorkflow.*`，窗口只负责启动命令、落盘 artifact 和刷新 UI；新增 `verification_workflow_smoke` CTest，覆盖无配置 testgrid、配置 testgrid、DRAW gate 失败和 patch apply 失败等关键状态转移。当前 workflow 服务仍只做状态决策，不直接执行命令或写文件。
- N34：新增 `core/geometry/TopologySignature.*`，使用 `TopExp` 遍历 object_id 加 `BRepTools::Write(subshape)` 的 SHA-256 生成拓扑签名映射。Viewer 可返回当前对象的 `stable_id` 和完整 signature JSON；几何 tab 新增“保存映射”，模型加载、Viewer 拾取和截图会写出 `artifacts/topology_signature_*.json` 并登记 Geometry Evidence。新增 `topology_signature_smoke` CTest，覆盖签名结构、对象签名和缺失对象错误路径。当前这是可审查的 shape dump/hash 映射，不是跨所有拓扑重建场景的永久命名算法。
- N35：testgrid/testdiff 结果新增结构化 `failure_details`、`timing` 和 `testdiff_artifacts`；单阶段与二阶段验证 artifact 都会记录失败明细、耗时节点和 testdiff summary/日志指针。VerificationReport 会优先消费这些字段并在 Markdown 中输出 Timing Summary 与 artifact 链接；EvidenceBundle 新增 `verification_failures`、`verification_timing`、`testdiff_artifacts`。新增 `verification_result_parser_smoke` CTest，现有 report/evidence smoke 已覆盖 N35 字段。
- N36：PatchCandidate 的 `git diff --binary HEAD` 生成已改为异步 `CommandRunner` 执行；stdout/stderr 分别写入 `logs/patch_generate.stdout.log` / `logs/patch_generate.stderr.log`，结构化结果写入 `artifacts/patch_generate_result.json`。成功且有 diff 时继续保存 `artifacts/candidate_patch.diff` 与 `candidate_patch_manifest.json`；无 diff 或失败时保留独立生成结果、Evidence、verification item、VerificationReport 和 EvidenceBundle 记录。新增 sample artifact，并由 report/evidence smoke 覆盖 N36 字段。
- N37：`TopologySignature` 新增 before/after signature JSON 匹配策略 `compare()`，优先按同类型 BREP SHA-256 精确匹配，再用 object_id、orientation、children、index 生成可审查近似分数，输出 `matches`、`unmatched_before`、`unmatched_after`、`counts_delta` 和 summary。`topology_signature_smoke` 已覆盖相同 shape 稳定匹配与变更 shape 非稳定匹配。当前仍不是跨所有拓扑重建场景的永久命名算法。
- N38：新增 `core/verify/TwoStageVerificationResultWriter.*`，把两阶段验证 phase result 与 final result 的 JSON 组装从 `WorkbenchWindow` 抽出；窗口仍负责文件读写、UI 刷新和 Evidence 编排。新增 `two_stage_verification_result_writer_smoke` CTest，覆盖 phase/final JSON 的 rows、failure_details、timing、testdiff artifacts、plan、phase artifact 和 before_after 字段。
- N39：新增 `core/verify/TestdiffArtifactScanner.*`，扫描 Case workspace 内约定的 `verification/testdiff/{before,after,diff}`、`verification/testdiff_*`、`artifacts/testdiff/{before,after,diff}`、`artifacts/testdiff_*` 目录，生成 `directories`、`artifact_files`、`artifact_counts` 和 `truncated` 字段；按 image/property/performance/log/text/other 分类归档真实 testdiff 工件。单阶段与二阶段 result JSON、VerificationReport、EvidenceBundle 和 Markdown 链接已消费该清单，sample case 与 smoke 测试已覆盖。
- N40：新增 `core/geometry/TopologyCompareArtifact.*`，统一读取 `artifacts/topology_compare.json`，并在缺少 compare artifact 但存在 before/after signature 时可用 `TopologySignature::compare()` 内存生成同构对象。几何差异 tab 会展示 topology compare 摘要；EvidenceBundle 新增 `geometry_diff`；VerificationReport JSON/Markdown 新增 Geometry Diff 区块与 topology compare artifact 链接。sample case 与 `verification_report_smoke`、`evidence_bundle_smoke` 已覆盖。
- N41：新增 `core/verify/TestgridArtifactService.*`，集中管理 Case workspace 下 `logs/`、`verification/`、`artifacts/` 的 testgrid 路径、目录创建、命令日志写入、phase summary 读写和 JSON artifact 写入。`WorkbenchWindow` 的单阶段 testgrid result、二阶段 phase result 与最终 result 已改用该服务，减少窗口层文件路径/读写职责；新增 `testgrid_artifact_service_smoke` CTest 固化路径与读写约定。UI 刷新和 Evidence 编排仍在窗口层，后续继续拆分。
- N42：新增 `core/verify/TestdiffRunnerAdapter.*`，把外部 testdiff runner 输出的 `before/after/diff` 或 `testdiff/{before,after,diff}` 目录复制归一化到 Case workspace 的 `artifacts/testdiff/{before,after,diff}`，再复用 `TestdiffArtifactScanner` 生成 manifest。adapter manifest 只记录 Case 相对目录、复制数量和工件清单，不泄露 runner 输出绝对路径；新增 `testdiff_runner_adapter_smoke` 覆盖图片/属性/性能工件导入。
- N43：`TopologyCompareArtifact` 已支持从用户选择的 before/after topology signature JSON 生成并落盘 `artifacts/topology_compare.json`；几何差异 tab 新增生成入口，成功后刷新差异摘要、登记 Geometry Evidence、同步 EvidenceBundle 与 VerificationReport。新增 `topology_compare_artifact_smoke` CTest，覆盖 compare artifact 写入、读取和本机绝对路径脱敏。
- N44：新增 `workbench/EvidenceCoordinator.*` 和 `workbench/TestgridTablePresenter.*`，把 EvidenceRecord 追加到 UI 数据/CaseManifest 的同步逻辑、testgrid rows 到表格单元的映射从 `WorkbenchWindow` 抽出。主窗口仍负责命令执行、artifact 生成和报告刷新；新增 `workbench_presenter_smoke` CTest 覆盖 Evidence 同步与 testgrid 表格模型。
- N45：`verification.testgrid_plan` 新增 `testdiff_arguments` 和 `testdiff_output_root`；底部 testgrid 面板新增 `Run testdiff` 入口，先执行 `draw_smoke` 门禁，再运行配置的 testdiff 命令，并通过 `TestdiffRunnerAdapter` 导入 runner 输出目录到 `artifacts/testdiff/{before,after,diff}`。命令日志、summary、adapter result/manifest 会落盘，并更新兼容入口 `artifacts/testgrid_result.json` 供 VerificationReport 和 EvidenceBundle 继续消费。新增 `case_manifest_plan_smoke` 覆盖新 plan 字段读写。
- N46：`TopologySignature` 已为每个记录补充局部几何和子拓扑统计，包括 bounding box、bbox center、bbox diagonal、线/面/体 measure、center of mass 与子形状计数；before/after 近似评分新增 measure、bbox center/diagonal 和 subshape counts 策略提示。`topology_signature_smoke` 已覆盖新增字段和增强匹配路径。当前仍是可审查的启发式匹配增强，不是完整永久命名算法。
- N47：新增 `core/verify/TestgridResultWriter.*`，把单阶段 testgrid result 的解析、verification items、diff summary、failure details、timing 和 `artifacts/testgrid_result.json` 组装/写入从 `WorkbenchWindow` 抽出。主窗口保留命令日志、UI 刷新、Evidence 登记和报告触发；新增 `testgrid_result_writer_smoke` CTest 覆盖 writer 的结果落盘与相对路径约定。
- N48：新增 `core/verify/TestdiffCommandPlanner.*`，把 testdiff executable 校验、输出目录规范化/创建、工作目录 fallback、占位符替换和 `CommandRequest` 组装从 `WorkbenchWindow` 抽出。主窗口只负责调用 planner、启动 runner 和记录最后输出目录；新增 `testdiff_command_planner_smoke` CTest 覆盖默认/相对输出目录、占位符和缺少 executable 的错误路径。
- N49：新增 `core/verify/TestdiffArtifactIndex.*`，把 testdiff scanner 已发现的 image/property/performance 工件按 normalized key 归并为 before/after/diff 索引，输出 `artifact_index`、kind/role 计数、配对状态和策略说明；`TestdiffArtifactScanner` 与 `TestdiffRunnerAdapter` manifest 已集成该索引，新增 `testdiff_artifact_index_smoke` CTest 覆盖索引语义。
- N50：新增 `workbench/TwoStageFinalResultCoordinator.*`，把二阶段 final result 的 WorkbenchMockData/CaseManifest 状态同步、verification items、Evidence 登记和 EvidenceBundle/VerificationReport 触发意图从 `WorkbenchWindow` 抽出；窗口层只负责控件刷新、JSON 写入和实际报告写出，`workbench_presenter_smoke` 已覆盖该同步路径。
- N51：新增 `core/verify/TestdiffAdapterResultWriter.*` 和 `workbench/TestdiffAdapterResultCoordinator.*`，把 testdiff runner 日志、summary、adapter result/manifest、兼容 `testgrid_result.json` 写入，以及 UI 数据/manifest/Evidence/报告刷新触发意图从 `WorkbenchWindow` 拆出；新增 `testdiff_adapter_result_writer_smoke` 并扩展 `workbench_presenter_smoke` 覆盖该路径。
- N52：VerificationReport 与 EvidenceBundle 已消费 `testdiff_artifacts.artifact_index`，输出 `artifact_index` 明细和 `artifact_index_summary` 摘要；Markdown 验证报告新增 Testdiff Artifact Index 小节展示 image/property/performance 的配对状态和关键工件组，`verification_report_smoke` / `evidence_bundle_smoke` 已覆盖。
- N53：新增 `core/verify/TwoStagePhaseResultWriter.*`，把二阶段 before/after phase 的 gate/command 日志写入、summary fallback 解析、phase summary 写入、failure/timing 构造和 `artifacts/testgrid_<phase>_result.json` 写入从 `WorkbenchWindow` 抽出；新增 `two_stage_phase_result_writer_smoke` 覆盖该路径。
- N54：新增 `core/verify/TestdiffArtifactAnalysis.*`，在不引入第三方依赖、不生成伪 diff 的前提下，对 runner 已提供的 image/property/performance 工件做轻量解析：图片记录 before/after/diff 可用性，属性 JSON 输出类型和顶层 key 摘要，性能文本提取简单数值指标；Scanner、adapter、单阶段/二阶段 result、VerificationReport 和 EvidenceBundle 均已消费 `artifact_analysis`，新增 `testdiff_artifact_analysis_smoke` 覆盖该路径。
- N55：新增 `workbench/DiffArtifactsPresenter.*`，把 `testdiff_artifacts.artifact_index` 与 `artifact_analysis` 转为稳定 UI 表格行；差异对比 tab 已新增工件索引表和轻量分析表，并在 Case 切换、单阶段 testgrid、testdiff adapter、二阶段 final 和 topology compare 后刷新；`workbench_presenter_smoke` 已覆盖行模型。
- N56：新增 `core/verify/TwoStageFinalResultWriter.*`，把二阶段 final result 的 `artifacts/testgrid_two_stage_result.json` 与兼容入口 `artifacts/testgrid_result.json` 写入从 `WorkbenchWindow` 抽出；窗口层先调用 writer 写最新 final artifact，再调用 coordinator 同步 UI 数据并刷新差异表，`two_stage_verification_result_writer_smoke` 已覆盖实际落盘。
- N57：新增 `core/verify/TestdiffGenerationPolicy.*`，把图片像素 diff、属性结构 diff、性能趋势 diff 的生成器边界固化为结构化 `generation_policy`：当前只评估候选输入、阻塞原因和后续契约，不启用生成、不写出伪 artifact；`artifact_analysis` 已携带该策略，新增 `testdiff_generation_policy_smoke` 并扩展 `testdiff_artifact_analysis_smoke` 覆盖。
- N58：差异对比 tab 已新增 artifact kind/status 过滤和 `Open artifact` 入口；索引表双击可按 diff/after/before 优先级打开 Case 相对工件，分析表双击可跳转到对应索引工件；打开逻辑限制在当前 Case workspace 内，文本/日志/JSON 预览到底部控制台，图片等二进制工件走系统关联程序。`workbench_presenter_smoke` 已覆盖过滤行模型与当前 `artifact_analysis` schema。
- N59：新增 `workbench/TwoStageFinalResultUiAdapter.*`，把二阶段 final 后 diff label、testgrid 表、VerificationPanel、EvidencePanel 的控件刷新和保存/报告触发动作从 `WorkbenchWindow` 手写块中抽出；窗口层仍负责 final writer、diff artifact 表刷新、Case 保存和 EvidenceBundle/VerificationReport 实际写出。`workbench_presenter_smoke` 已覆盖 UI adapter 的控件刷新与触发动作。
- N60：新增 `core/verify/TestdiffGenerationContract.*`，定义真实 testdiff 生成器的 opt-in manifest 字段、Case 相对输出根目录、sidecar 后缀、三类生成器输出命名和隐私边界；`TestdiffGenerationPolicy` 已在 `generation_policy.contract` 中携带该契约，但仍保持 `enabled=false`、`generation_performed=false`，不生成伪 artifact。新增 `testdiff_generation_contract_smoke`，并扩展 policy/analysis smoke 覆盖集成路径。
- N61：新增 `workbench/DiffPanel.*`，把 Diff Compare tab 的摘要 label、artifact kind/status 过滤、索引表、分析表、打开按钮和双击跳转从 `WorkbenchWindow` 拆出；窗口层只负责读取最新 `testgrid_result.json`、把 `testdiff_artifacts` 喂给 panel，以及执行 Case workspace 内的安全打开/预览逻辑。`workbench_presenter_smoke` 已覆盖 DiffPanel 的摘要刷新、表格刷新和首选 artifact 路径选择。
- N62：新增 `core/verify/TwoStageFinalResultBuilder.*`，把二阶段 final 的 before/after rows、comparison、testdiff summary、failure_details、timing 和 `testdiff_artifacts` 输入组装从 `WorkbenchWindow` 抽出；窗口层只负责调用 builder、writer/coordinator、Case 保存和报告实际写出。新增 `two_stage_final_result_builder_smoke` 覆盖 writer input、回归对比、失败明细、耗时、testdiff artifact 扫描和本机绝对路径不泄露。
- N63：新增 `workbench/ReportRefreshCoordinator.*`，把 EvidenceBundle 与 VerificationReport 的实际写出、路径计算和错误汇总从 `WorkbenchWindow` 抽出；窗口层只提交刷新请求并把错误输出到控制台。新增 `report_refresh_coordinator_smoke` 覆盖双报告写出、输出路径和 workspace 边界，既有 evidence/report smoke 保持通过。
- N64：DiffPanel 已补充 artifact 搜索框、路径复制和图片内嵌预览入口；搜索过滤已扩展到 kind/key/status/path/analysis 摘要，图片预览由 `WorkbenchWindow` 复用 Case workspace 安全路径校验后加载 `QPixmap`，DiffPanel 本身不直接读任意文件。`workbench_presenter_smoke` 已覆盖搜索过滤和预览状态。
- N65：新增 `scripts/build.ps1` 与 `scripts/run.ps1`，统一本地 configure/build/CTest 和启动入口；脚本会通过 `OCCTDEBUG_VSDEVCMD`、`vswhere` 或当前 VS 环境发现 developer command，不写死个人机器路径。
- N66：`CommandRunner` 已补充 `CommandResult::canceled` 语义和 `command_runner_cancel_smoke`；DRAW、环境采集、testgrid/testdiff/二阶段验证和 patch 命令已有最小取消按钮，取消 testgrid/patch 编排时不会继续后续阶段。
- N67：加载 demo 或导入模型后，OCCT Viewer 的 `topologySummary()` 会同步为几何检查表 `Topology stats` 行并写回 Case manifest，补齐当前阶段 Shape 基础统计展示。
- N68：`CaseManifest` 新增 `input.files`，几何模型导入成功后记录 Case 相对路径、原始文件名、bytes、SHA-256 和导入时间；`repro_report.md` 新增输入文件摘要表，`case_manifest_plan_smoke` 与 `markdown_report_generator_smoke` 覆盖该契约。
- N69：新增 `core/repro/CppReproTemplateWriter.*` 与 UI `C++ Repro` 入口，可在当前 Case `repro/cpp_minimal/` 生成最小 CMake/C++ 复现工程、README 和 DRAW 脚本副本；`cpp_repro_template_writer_smoke` 覆盖模板输出和本机绝对路径不泄露。
- N70：`CommandRunner` 已补充 `timeoutMs` 输入、`timedOut` 结果语义和 `command_runner_timeout_smoke`；DRAW、环境采集、testgrid/testdiff、二阶段验证和 patch 命令已有统一超时配置，UI 日志与结果 artifact 可区分 passed/failed/canceled/timed_out。
- N71：新增 `core/case/CrashDumpArchive.*` 与 Evidence 面板 `Archive dump` 入口，可将 `.dmp/.mdmp/.dump` 归档到当前 Case `artifacts/crash/`，写出 SHA-256 manifest 并登记 Evidence；`crash_dump_archive_smoke` 覆盖归档输出和本机绝对路径不泄露。
- N72：`CaseManifest` 新增 `repro.status`，`ReproStatusEvaluator` 统一判定 DRAW/C++/testgrid 复现状态；DRAW 运行、C++ 模板生成和 testgrid 落盘会写回 Case，并用 `repro_status_evaluator_smoke` 与 `case_manifest_plan_smoke` 覆盖。
- N73：`CaseManifest` 新增 `verification.testdiff_generation`，`TestdiffGenerationContract` 补充默认 opt-in 配置、容差/阈值和失败报告 schema；`TestdiffGenerationPolicy` 可识别显式 opt-in 并输出有效配置与 `failure_report` 状态，但仍保持 `enabled=false`、`generation_performed=false`，不生成伪 artifact。新增 `testdiff_generation_failure_report_smoke`，并扩展 contract/policy/case manifest smoke 覆盖。

后续任务应优先把这些基础能力串成真实 Case 工作流。

## 2. 推荐任务顺序

```text
N74 继续收束 WorkbenchWindow 中剩余 Case 保存、命令编排和 UI 刷新职责
N75 完善命令任务队列/状态面板，把当前最小状态日志升级为可浏览的任务历史
N76 为真实 testdiff 生成器输出 sidecar 与 failure report writer 后，再启用具体生成算法
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

## 11. N9 / N15：OCCT Viewer 加载模型

```text
任务：让 OCCT Viewer 从基础集成进入可用状态。

范围：
- 支持加载 BREP、STEP 和 IGES。
- 支持 fit all、基础显示、清空视图。
- 不实现复杂选择和修复。

验收：
- sample 小模型可加载显示，文件格式不支持或 OCCT 读取失败时 UI 不崩溃。
- 文件不存在或格式错误时 UI 不崩溃。
```

## 12. N10：testgrid/testdiff 接入

```text
任务：实现 testgrid/testdiff 的最小 runner。

范围：
- 所有 testgrid/testdiff 测试前要求 draw_smoke 通过。
- 支持配置 root/executable/arguments/group/grid/case。
- 支持运行一个配置好的最小测试命令；未配置 executable 时降级为门禁和 summary 解析。
- 解析通过率、失败列表、日志路径。

不要做：
- 不接入全量大型回归。
- 不要求联网。

验收：
- draw_smoke 失败时不启动 testgrid。
- 结果能显示到底部 testgrid 面板。
- 结果能写入当前 Case `artifacts/testgrid_result.json`。
```

## 13. N17：候选补丁审查报告

```text
任务：实现候选补丁人工审查的最小闭环。

范围：
- CaseManifest 记录 patch.review_status 和 patch.review_items。
- UI 允许将候选补丁标记为送审、通过或退回。
- 导出 report/patch_review.md。
- 将审查报告登记为 Evidence。

不要做：
- 不应用 patch 到 OCCT worktree。
- 不撤销 patch。
- 不生成自动补丁。

验收：
- 审查状态能保存到当前 Case manifest。
- 重启或切换 case 后能恢复审查状态。
- 审查报告能重复导出。
- 构建、CTest 和启动检查通过。
```

## 14. Codex 代码审查任务

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
