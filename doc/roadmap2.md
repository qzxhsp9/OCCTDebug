# OCCT 内核专家工作台 Roadmap

> 项目代号：**OCCT Kernel Expert Workbench**  
> 当前阶段：Qt Widgets 工作台骨架已完成，Case/Runner/DRAW/Viewer/Evidence/Report/Verify 等基础模块已启动，正在收束为真实 Case 工作流
> 目标平台：**Windows x64**  
> UI 技术栈：**Qt Widgets + C++**  
> 内核技术栈：**OCCT + CMake + MSVC/Visual Studio + DRAW Test Harness + testgrid**  
> 参考资料目录：`doc/`；当前主路线图：`doc/roadmap2.md`；旧路线图：`doc/roadmap1.md`（弃用，仅作历史参考）

---

## 1. 项目目标

本项目目标是开发一款面向 OCCT 内核开发人员的本地化专家工作台，用于支持从问题录入、数据准备、自动复现、源码分析、补丁生成、回归验证到知识归档的完整闭环。

该工具不以“聊天式问答”为核心，而以**工程化问题处理流水线**为核心：

```text
问题描述 → 环境采集 → 复现生成 → 证据收集 → 根因分析 → 补丁方案 → 回归验证 → 知识归档
```

最终工具应能帮助企业开发人员高效处理以下 OCCT 相关问题：

- OCCT 算法崩溃、异常、空指针、内存错误。
- Boolean、Fillet、Chamfer、Offset、Section 等建模算法失败。
- STEP / IGES / XCAF 数据交换异常。
- Shape Healing、拓扑修复、Tolerance 异常。
- DRAW 脚本复现与最小化复现用例构建。
- OCCT 源码定位、调用栈分析、历史问题检索。
- 自动生成补丁候选、测试用例和验证报告。
- 将企业内部问题沉淀为可复用知识库。

---

## 2. 建设原则

### 2.1 工程闭环优先

所有问题必须尽量形成可执行、可验证、可归档的工程闭环。工具输出不应停留在分析建议，而应尽量包含：

```text
case manifest + 输入数据 + 复现脚本 + 运行日志 + 证据链 + 补丁候选 + 验证结果 + 归档记录
```

### 2.2 复现优先于修复

没有稳定复现的问题，不进入自动补丁流程。工具应优先完成：

1. 原始问题复现。
2. 标准化复现。
3. 最小化复现。
4. 复现报告生成。

### 2.3 本地数据安全优先

企业 CAD 数据默认视为敏感数据。工具应默认本地运行，不自动外传源码、模型、日志或报告。

### 2.4 人机协同，而非全自动黑盒

工具允许自动分析和自动生成候选补丁，但关键节点应支持人工确认：

- 是否接受最小化后的数据。
- 是否接受根因判断。
- 是否采用候选补丁。
- 是否执行大范围回归测试。
- 是否归档到企业知识库。
- 是否准备提交 upstream。

### 2.5 UI 简洁、清晰、实用、高效

界面应以“内核专家工作台”为目标，避免做成复杂海报式界面。每个区域都应直接服务于开发者的实际工作：看源码、看模型、看日志、看证据、看补丁、看验证结果。

---

## 3. 产品最终形态

最终应用是一个 Windows 桌面程序，主界面由五个核心工作区组成：

```text
┌──────────────────────────────────────────────────────────────────────┐
│ 顶部标题栏 / 全局工具栏                                               │
├───────────────┬──────────────────────────────────────┬───────────────┤
│ 左侧案例区     │ 中央专家工作区                         │ 右侧决策区     │
│ Case Explorer │ 源码 / 复现脚本 / 几何 / 证据 / 差异       │ 诊断 / 补丁 / 验证 │
├───────────────┴──────────────────────────────────────┴───────────────┤
│ 底部执行控制台：DRAW / PowerShell-CMake / testgrid                    │
└──────────────────────────────────────────────────────────────────────┘
```

主要能力包括：

- 案例管理。
- 环境快照。
- DRAW / C++ 复现。
- OCCT 几何可视化。
- 源码与调用栈联动。
- 几何检查与 Shape 信息查看。
- 证据链组织。
- 根因结论与置信度展示。
- 候选补丁审查。
- testgrid / testdiff 验证。
- 报告导出与知识归档。

---

## 4. 当前仓库结构与目标目录结构

本仓库不是全新空目录，当前根目录以实际 clone 位置为准，文档中统一用占位符表示：

```text
<repo-root>/OCCTDebug/
```

当前已经完成一次旧代码清理：旧模型查看器、旧 Shape 树、旧诊断规则、旧 IO、旧会话系统和旧 problem document importer 均已删除。需要旧实现时从 git 历史查找，不在当前源码树中保留无用代码。

当前新的工作台基础已经不再停留在静态 UI：Case 数据模型、环境采集、统一命令执行、DRAW smoke CTest、DRAW 日志解析、临时 CTest 注册、Repro Pack、OCCT Viewer、Evidence 面板、Markdown 报告、testgrid/testdiff 解析、源码索引、相似案例检索和候选补丁审查均已有第一版基础实现。后续重点是把这些能力接入统一 Case workspace，而不是继续扩展离散 mock。

### 4.1 当前实际结构

当前源码结构应以此为准：

```text
OCCTDebug/
  CMakeLists.txt
  CMakePresets.json
  README.md
  AGENTS.md

  cmake/
    occt_setup_install.cmake
    occt_3rdpart_setup_install.cmake

  depends/
    occt/
    occt_3rdparty/
      freetype-2.13.3-x64/
      tcltk-8.6.15-x64/

  doc/
    OCCT_AutoFix_Workbench_Design.md
    OCCT_Kernel_Expert_Workbench_UI_Design.md
    CODEX_CONTEXT.md
    CODEX_TASKS.md
    DEVELOPMENT_CHECKLIST.md
    DRAW_SMOKE_CTEST.md
    occt内核专家工作台分析界面.png
    roadmap1.md                 # 旧路线图，弃用
    roadmap2.md                 # 当前路线图，后续维护此文件

  src/
    CMakeLists.txt
    QtWorkbenchDefaults.cmake.example
    app/
      main.cpp
    core/
      Logger.h
      Logger.cpp
      case/
      geometry/
      knowledge/
      patch/
      report/
      runner/
      source/
      verify/
    workbench/
      CasePanel.h
      CasePanel.cpp
      EvidencePanel.h
      EvidencePanel.cpp
      SourcePanel.h
      SourcePanel.cpp
      VerificationPanel.h
      VerificationPanel.cpp
      WorkbenchWindow.h
      WorkbenchWindow.cpp
      WorkbenchMockData.h
      WorkbenchMockData.cpp

  tests/
    CMakeLists.txt
    draw_smoke.tcl
    draw_checkshape_smoke.tcl

  scripts/
    verify_env.ps1
    run_draw_smoke.ps1
    parse_draw_log.ps1
    register_temp_draw_ctest.ps1
    export_repro_pack.ps1
    update_codex_context.ps1

  data/                         # 样例数据，可继续保留和扩展
  knowledge/                    # 后续知识库占位
  out/build/debug/              # 当前本地 Debug 构建目录
```

### 4.2 近期目标结构

在当前结构上增量扩展，不再恢复旧目录。建议后续按新架构逐步增加：

```text
src/
  app/
    main.cpp
  workbench/                    # Qt 主界面、面板、对话框
    WorkbenchWindow.*
    panels/
    dialogs/
    widgets/
  core/
    Logger.*
    app/                        # AppContext、全局服务注册
    config/                     # 配置模型与加载器
    case/                       # CaseManifest、WorkflowState
    env/                        # 环境采集
    repro/                      # 复现资产生成
    geometry/                   # OCCT 模型加载、Viewer 适配、Shape 检查
    source/                     # 源码定位与索引
    evidence/                   # EvidenceBundle 与证据链
    diagnosis/                  # 根因假设与相似案例
    patch/                      # patch diff、候选补丁、审查
    verify/                     # CTest/testgrid/testdiff 验证
    report/                     # Markdown/HTML/JSON 报告
    knowledge/                  # 本地知识库模型
  runners/
    DrawRunner.*
    CMakeRunner.*
    CTestRunner.*
    TestgridRunner.*
    PowerShellRunner.*

config/                         # 后续新增
  workbench.default.yaml
  workbench.local.example.yaml
  workbench.local.yaml           # 本地配置，加入 .gitignore

cases/                           # 后续新增，本地 case workspace，可 gitignore
knowledge/                       # 当前已存在，后续扩展 index/cache/private_cases
```

原则：目录随着功能落地再创建，不预先堆空目录；每个新增目录必须有明确职责和至少一个真实调用点。

## 5. 当前本地配置与后续配置文件规划

本节是 `roadmap2.md` 需要重点维护的本地事实。当前项目已经能通过仓库内 `depends/` 和本地 Qt kit 构建运行。

### 5.1 当前已生效的本地配置

当前仓库根目录：

```text
<repo-root>/OCCTDebug
```

当前 Debug 构建目录：

```text
<repo-root>/OCCTDebug/out/build/debug
```

当前应用输出：

```text
<repo-root>/OCCTDebug/out/build/debug/src/OCCTDebug.exe
```

当前 OCCT 配置来自：

```cmake
cmake/occt_setup_install.cmake
```

该脚本固定从仓库内读取：

```text
depends/occt
```

按 `CMAKE_BUILD_TYPE` 选择：

| 构建类型 | OCCT lib 目录 | OCCT dll 目录 |
|---|---|---|
| Debug | `depends/occt/lib/Debug/libd` | `depends/occt/lib/Debug/bind` |
| Release | `depends/occt/lib/Release/lib` | 当前脚本仍指向 `depends/occt/lib/Debug/bin`，后续需要核对修正 |
| RelWithDebInfo | `depends/occt/lib/RelWithDebInfo/libi` | `depends/occt/lib/RelWithDebInfo/bini` |

当前 FreeType 配置来自：

```cmake
cmake/occt_3rdpart_setup_install.cmake
```

该脚本固定从仓库内读取：

```text
depends/occt_3rdparty/freetype-2.13.3-x64
```

当前 Tcl/Tk 配置来自：

```text
depends/occt_3rdparty/tcltk-8.6.15-x64
```

该目录用于支撑 `DRAWEXE.exe` 运行，`scripts/run_draw_smoke.ps1` 会为 DRAW 进程设置 `TCL_LIBRARY`、`TK_LIBRARY`，并将 Tcl/Tk bin/lib 加入进程级 PATH。不要依赖系统全局 Tcl/Tk。

当前 Qt 配置入口：

```text
src/QtWorkbenchDefaults.cmake
```

当前本机 Qt kit 应写入本地忽略文件，不写入共享文档或源码：

```text
<qt-root>/msvc2022_64
```

模板文件：

```text
src/QtWorkbenchDefaults.cmake.example
```

说明：`src/QtWorkbenchDefaults.cmake` 是本地文件，已被 `.gitignore` 忽略；不要把个人 Qt 安装路径提交到仓库。

当前 CMake presets：

```text
CMakePresets.json
CMakeUserPresets.json
```

其中 `CMakeUserPresets.json` 属于本地配置文件，应继续保持 gitignore。当前机器上该文件可能仍包含 Qt Creator 生成的旧 kit 信息，例如 Qt 6.6.2；它不应作为团队共享事实。团队共享配置应优先放在 `CMakePresets.json`，个人路径放在 `src/QtWorkbenchDefaults.cmake` 或未来的 `config/workbench.local.yaml`。

团队共享 Debug preset：

```powershell
cmake --preset OCCTDebug-Debug
cmake --build --preset OCCTDebug-Debug
ctest --preset OCCTDebug-Debug
```

当前可用验证命令：

```powershell
cmd /c ""<vsdevcmd-path>" -arch=x64 -host_arch=x64 && cmake --build out\build\debug --config Debug && ctest --test-dir out\build\debug --output-on-failure"
```

当前 DRAW smoke 验证命令：

```powershell
ctest --test-dir out\build\debug -R "draw_.*smoke" --output-on-failure
```

当前 DRAW CTest：

| CTest | 用途 | 标签 |
|---|---|---|
| `draw_smoke` | 验证 DRAWEXE 能启动并执行最小 Tcl | `draw;smoke;occt` |
| `draw_checkshape_smoke` | 创建内存 box 并执行最小 `checkshape` | `draw;smoke;occt;checkshape` |

`draw_smoke` 提供 CTest fixture `draw_ready`，后续 testgrid/testdiff CTest 必须依赖该 fixture，将 DRAW 环境作为前置门禁。

### 5.2 后续应用配置文件

后续新增 `config/` 时，建议采用以下约定：

```text
config/workbench.default.yaml          # 可提交，存默认行为
config/workbench.local.example.yaml    # 可提交，说明字段
config/workbench.local.yaml            # 不提交，存个人路径
```

`config/workbench.local.yaml` 示例应适配当前仓库：

```yaml
workspace:
  repo_root: "<repo-root>/OCCTDebug"
  build_root: "<repo-root>/OCCTDebug/out/build"
  case_root: "<repo-root>/OCCTDebug/cases"
  artifact_root: "<repo-root>/OCCTDebug/artifacts"

occt:
  bundled_root: "<repo-root>/OCCTDebug/depends/occt"
  source_root: ""
  build_root: ""
  install_root: "<repo-root>/OCCTDebug/depends/occt"
  casroot: "<repo-root>/OCCTDebug/depends/occt"
  drawexe: ""
  testgrid_root: ""

third_party:
  freetype_root: "<repo-root>/OCCTDebug/depends/occt_3rdparty/freetype-2.13.3-x64"
  tcltk_root: "<repo-root>/OCCTDebug/depends/occt_3rdparty/tcltk-8.6.15-x64"

qt:
  root: "<qt-root>/msvc2022_64"

compiler:
  developer_command: "<vsdevcmd-path>"
  generator: "Ninja"
  architecture: "x64"

runtime:
  default_build_type: "Debug"
  relwithdebinfo_enabled: true
  enable_asan: false
  max_parallel_jobs: 12

privacy:
  default_case_level: "internal"
  allow_external_network: false
```

注意：`drawexe` 当前可由 `scripts/run_draw_smoke.ps1` 和 `scripts/verify_env.ps1` 自动从仓库内 OCCT 布局检测；`source_root` 仍未接入完整源码索引配置。`testgrid_root`、`testgrid_executable` 和 group/grid/case 已有最小配置化 runner 入口，未配置 executable 时只执行 `draw_smoke` 门禁和 summary 解析。

### 5.3 默认配置文件

`config/workbench.default.yaml` 后续用于提交默认参数、UI 偏好、日志等级和 runner 默认行为：

```yaml
ui:
  theme: "dark"
  language: "zh-CN"
  autosave_interval_sec: 30

logging:
  level: "info"
  keep_days: 30

repro:
  prefer_draw_script: true
  generate_cpp_repro: true
  enable_shape_minimize: false

verify:
  run_related_tests_first: true
  run_full_testgrid: false
  performance_compare: true

patch:
  worktree_root: ""
```

实现顺序：先实现配置模型和读取默认值，再增加本地配置文件；不要在 UI 逻辑中散落硬编码路径。

## 6. 核心数据模型

### 6.1 CaseManifest

每个问题对应一个 `case.json` 或 `case.yaml`。

```yaml
case_id: "OCC-LOCAL-2026-0001"
title: "Fillet 更新失败，Null curve 导致崩溃"
status: "diagnosing"
category: "modeling/fillet"
created_at: "2026-06-03T10:00:00-04:00"
updated_at: "2026-06-03T10:00:00-04:00"

occt:
  version: "7.8.1"
  commit: ""
  build_type: "RelWithDebInfo"

input:
  model_files:
    - "input/valve_body_min.brep"
  repro_type: "DRAW"
  confidential_level: "internal"

failure:
  symptom: "崩溃 / Null curve"
  expected: "Fillet 正常生成或返回明确错误"
  actual: "BRep_Tool::Curve 返回空曲线后续访问异常"
  deterministic: true

artifacts:
  env_snapshot: "env/env_snapshot.json"
  repro_script: "repro/repro.tcl"
  crash_log: "logs/crash.log"
  report: "report/diagnosis.md"
```

### 6.2 EnvironmentSnapshot

```yaml
windows:
  version: "Windows 11 x64"
  cpu: ""
  memory: ""

compiler:
  visual_studio: "VS2022"
  msvc_version: ""
  cmake_version: ""

occt:
  source_root: ""
  build_root: ""
  casroot: ""
  drawexe: ""
  third_party_paths: []

environment_variables:
  CASROOT: ""
  PATH: ""
  CSF_DEBUG_BOP: ""
```

### 6.3 ExecutionRecord

```yaml
run_id: "run-0001"
case_id: "OCC-LOCAL-2026-0001"
type: "draw"
command: "DRAWEXE.exe < repro.tcl"
start_time: ""
end_time: ""
exit_code: 1
result: "failed"
logs:
  stdout: "logs/run-0001.stdout.log"
  stderr: "logs/run-0001.stderr.log"
artifacts:
  screenshot: "artifacts/repro.png"
  dump: "artifacts/crash.dmp"
```

### 6.4 EvidenceRecord

```yaml
evidence_id: "ev-0001"
type: "crash_stack"
summary: "BRep_Tool::Curve 返回空曲线，后续 Fillet 更新崩溃"
confidence: 0.86
related_files:
  - "src/BRepFilletAPI/BRepFilletAPI_MakeFillet.cxx"
  - "src/BRepLib/BRepLib.cxx"
related_shapes:
  - "E125"
```

### 6.5 PatchCandidate

```yaml
patch_id: "patch-0001"
case_id: "OCC-LOCAL-2026-0001"
status: "draft"
summary: "在 Fillet 更新前增加空曲线检测并返回明确错误"
risk_level: "medium"
files_changed:
  - "src/BRepFilletAPI/BRepFilletAPI_MakeFillet.cxx"
test_cases:
  - "tests/bugs/modalg/bug_local_0001"
verification:
  original_fixed: true
  related_tests_passed: "33/35"
  testgrid_passed: "293/298"
  performance_delta: "+0.8%"
```

---

## 7. 总体路线图

路线图按功能成熟度拆分为 10 个阶段。每个阶段都应形成可运行、可验收的增量版本。

```text
P0 项目骨架与基础设施
P1 主界面与案例管理
P2 环境采集与配置管理
P3 复现生成与执行引擎
P4 几何查看与 Shape 检查
P5 源码定位与证据链
P6 诊断结论与相似案例
P7 补丁方案与代码审查
P8 回归验证与测试报告
P9 知识归档与案例复用
P10 产品化、插件化与稳定性增强
```

---

## 8. P0：项目骨架与基础设施

### 当前状态

状态：**基本完成，下一步补配置系统与 AppContext**。

P0 已经完成一次关键调整：旧实现已经清理，当前源码树只保留新工作台骨架和最小基础设施。

已完成：

- [x] 顶层 `CMakeLists.txt`。
- [x] `cmake/` 中 OCCT / FreeType 查找脚本。
- [x] Qt Widgets 应用入口 `src/app/main.cpp`。
- [x] 新工作台主界面骨架 `src/workbench/WorkbenchWindow.*`。
- [x] 左侧 Case 面板拆分为 `src/workbench/CasePanel.*`。
- [x] 中央源码面板拆分为 `src/workbench/SourcePanel.*`。
- [x] 中央证据链面板拆分为 `src/workbench/EvidencePanel.*`。
- [x] 右侧验证结果面板拆分为 `src/workbench/VerificationPanel.*`。
- [x] 最小日志工具 `src/core/Logger.*`。
- [x] DRAW smoke CTest：`draw_smoke`、`draw_checkshape_smoke`。
- [x] DRAW 运行辅助脚本：日志捕获、result JSON、日志解析、临时 CTest 注册、Repro Pack 导出。
- [x] 删除旧 GUI、旧诊断、旧 IO、旧会话、旧 Shape 树代码。
- [x] `README.md` 已说明当前重搭状态。

仍需完成：

- [ ] 统一 `CMakePresets.json`，避免团队共享配置依赖本地 `CMakeUserPresets.json`。
- [ ] 建立 `config/workbench.default.yaml`。
- [ ] 建立 `config/workbench.local.example.yaml`。
- [ ] 明确 `.gitignore` 中本地配置、case workspace、artifact、cache 的规则。
- [ ] 实现 `AppContext`，统一管理配置、路径、服务对象。
- [ ] 实现基础配置加载器。
- [ ] 将 UI/Runner 日志统一落盘到 Case workspace。
- [x] 已复核 `cmake/occt_setup_install.cmake`：Release 使用 `lib/Release/bin/*.dll`，Debug 使用 `lib/Debug/bind/*.dll`，当前无需代码修正。

### 交付物

- 可启动的 Qt Widgets 工作台应用。
- 项目基础目录清晰、无旧代码干扰。
- 本地配置文件规划明确。
- OCCT 与 Qt 基础链接验证。

### 验收标准

- [x] 应用能在 Windows x64 上构建。
- [x] CTest smoke 能验证 DRAWEXE 启动和最小 checkshape。
- [x] 启动入口是新的 `WorkbenchWindow`。
- [ ] 能读取默认配置和本地配置。
- [x] `verify_env.ps1` 能输出 OCCT、Qt、MSVC、CMake、DRAWEXE 和 DRAW smoke 状态。
- [ ] UI 环境 tab 能展示最新环境快照。
- [ ] UI/Runner 日志文件能按 Case 稳定写入。

## 9. P1：主界面与案例管理

### 当前状态

状态：**骨架已完成，已有 mock/sample 数据和真实 Case workspace；Case、源码、证据和验证面板已拆分为独立 widget，后续重点转向补丁/验证/知识闭环**。

当前 `WorkbenchWindow` 已经按 UI 设计图完成主框架：顶部状态栏、流程工具栏、中间源码/几何/证据/差异/环境 tab、右侧诊断/补丁/验证/相似案例、底部 DRAW/CMake/testgrid 控制台。左侧案例/流程/关键输入已由 `CasePanel` 独立承载，源码搜索/跳转由 `SourcePanel` 承载，证据列表由 `EvidencePanel` 承载，验证指标由 `VerificationPanel` 承载。

### 主要任务

已完成：

- [x] 实现 `WorkbenchWindow` 主框架。
- [x] 实现顶部标题栏与全局工具栏。
- [x] 实现左侧 `Case Explorer` 静态样例。
- [x] 实现左侧流程状态 stepper 静态样例。
- [x] 实现中央 tab 工作区。
- [x] 实现右侧决策支持面板。
- [x] 实现底部 console tab 区。
- [x] 实现暗色主题初版。
- [x] 实现 mock 数据提供层 `WorkbenchMockData`。

待完成：

- [x] 拆分左侧 `CasePanel`，避免 `WorkbenchWindow.cpp` 继续承载所有面板。
- [x] 拆分右侧 `VerificationPanel`，将验证指标展示和导出按钮从主窗口移出。
- [x] 拆分中央 `EvidencePanel`，将证据摘要和证据表刷新/追加从主窗口移出。
- [x] 拆分中央 `SourcePanel`，将搜索输入、源码文本和搜索结果列表从主窗口移出。
- [x] 引入 `CaseManifest` 基础数据模型。
- [x] 补齐 `WorkflowState` 数据模型基础。
- [x] 将静态样例 case 改为真实 case 数据。
- [x] 实现 case 创建、打开、保存。
- [x] 实现左侧 case 列表状态刷新；筛选仍待补齐。
- [x] 实现布局保存与恢复基础：主 splitter 尺寸、中心 tab、底部 tab 和底部高度。
- [ ] 实现状态栏、任务通知和全局消息提示。

### 主界面区域

```text
WorkbenchWindow
  ├── TopHeader
  │   ├── AppTitle
  │   ├── CaseBadge
  │   ├── EnvironmentBadges
  │   └── StatusBadge
  ├── MainToolbar
  ├── LeftSidebar
  │   ├── CaseListPanel
  │   ├── WorkflowPanel
  │   └── KeyInputPanel
  ├── CenterWorkspace
  │   ├── SourceTab
  │   ├── ReproScriptTab
  │   ├── GeometryTab
  │   ├── DiffTab
  │   └── EnvironmentTab
  ├── RightDecisionPanel
  │   ├── DiagnosisCard
  │   ├── PatchCard
  │   ├── VerificationCard
  │   └── SimilarCasesCard
  └── BottomConsole
      ├── DrawConsoleTab
      ├── PowerShellCMakeTab
      └── TestgridResultTab
```

### 交付物

- 完整可交互 UI 框架。
- Case 数据模型与样例 case 加载。
- 示例状态流转展示。
- 布局持久化。

### 验收标准

- [x] 主界面结构与 `OCCT_Kernel_Expert_Workbench_UI_Design.md` 方向一致。
- [x] 中央 tab 能正常切换。
- [x] 右侧卡片能展示示例诊断、补丁、验证信息。
- [x] 底部控制台能展示静态样例日志。
- [ ] 左侧案例能选择并刷新中央区域。
- [ ] 流程状态能根据 case 状态变化。
- [x] 底部控制台已有外部命令输出基础。
- [ ] 底部控制台输出按 Case artifacts 持久化。

## 10. P2：环境采集与配置管理

### 目标

实现对 Windows、OCCT、Qt、CMake、MSVC、DRAWEXE、testgrid 等环境的自动检测和快照保存。

### 当前状态

状态：**脚本级环境采集已可用，配置系统和 UI 配置对话框未完成**。

### 主要任务

- [ ] 实现环境配置对话框。
- [ ] 检测 OCCT 源码目录。
- [x] 检测仓库内 OCCT install 目录。
- [x] 检测 `DRAWEXE.exe`。
- [x] 检测 DRAW resources。
- [x] 检测 CMake。
- [x] 检测 MSVC / Visual Studio。
- [x] 检测 Qt 运行环境。
- [x] 检测 Tcl/Tk、FreeType 等常见依赖。
- [x] 读取 DRAW smoke / checkshape smoke CTest 结果。
- [ ] 保存 `env_snapshot.json` 到当前 Case。
- [ ] 在 UI 中展示真实环境检查结果。

### 对话框

#### 环境配置对话框

字段：

- OCCT 源码目录。
- OCCT 构建目录。
- OCCT 安装目录。
- DRAWEXE 路径。
- testgrid 根目录。
- CMake 路径。
- Visual Studio 版本。
- Qt 路径。
- 默认 build type。
- 默认 workspace 目录。

按钮：

- 自动检测。
- 手动选择。
- 验证配置。
- 保存。
- 取消。

### 交付物

- 环境检测服务。
- 环境快照文件。
- 环境配置 UI。
- 环境问题提示。

### 验收标准

- [x] 能检测 OCCT 与 Qt 当前配置。
- [x] 能检测 DRAWEXE 是否可运行。
- [x] 能检测 CMake / MSVC 是否可用。
- [x] 能导出环境快照 JSON。
- [x] 配置错误时脚本能给出明确提示。
- [ ] UI 能触发采集并展示错误提示。

---

## 11. P3：复现生成与执行引擎

### 目标

实现最重要的工程闭环基础能力：从 case 输入生成标准复现目录，并执行 DRAW / C++ 复现。

### 当前状态

状态：**DRAW 执行、输入文件 hash 和 C++ 复现模板基础已完成，Case workspace 闭环仍需继续收束**。

### 主要任务

- [ ] 实现新建问题对话框。
- [ ] 实现 case workspace 初始化。
- [x] 实现输入文件导入与 hash 记录：`CaseManifest.input.files` 已记录 Case 相对路径、原始文件名、bytes、SHA-256 和导入时间，几何模型导入成功后自动写入。
- [x] 实现 DRAW 脚本编辑器基础入口。
- [x] 实现 C++ 最小复现工程模板：UI 可在当前 Case `repro/cpp_minimal/` 生成最小 CMake/C++ 工程、README 和 DRAW 脚本副本。
- [x] 实现 DRAWEXE runner / CTest wrapper 基础。
- [x] 实现 PowerShell / CMake runner 基础。
- [x] 实现 DRAW 运行日志捕获。
- [x] 实现退出码和失败提示基础。
- [x] 完善 UI 侧超时、取消和任务状态。
- [x] 实现 crash dump 文件归档。
- [x] 实现复现状态判定。
- [x] 实现 Markdown 报告生成骨架。
- [ ] 将复现报告写入真实 Case workspace。

### 标准 case 目录

```text
cases/OCC-LOCAL-2026-0001/
  case.yaml
  input/
    original_model.step
    valve_body_min.brep
  repro/
    repro.tcl
    repro.cpp
    CMakeLists.txt
    run_repro.ps1
  env/
    env_snapshot.json
  logs/
    draw.stdout.log
    draw.stderr.log
    repro.log
  artifacts/
    crash.dmp
    repro.png
    shape_before.brep
    shape_after.brep
  report/
    repro_report.md
```

### 新建问题对话框

字段：

- 案例标题。
- 问题类型。
- OCCT 版本。
- 输入模型。
- 原始代码片段。
- 期望行为。
- 实际行为。
- 是否稳定复现。
- 保密级别。

按钮：

- 创建 case。
- 导入模型。
- 粘贴代码。
- 生成初始复现。
- 取消。

### 交付物

- case 创建流程。
- DRAW 复现执行。
- C++ 复现模板。
- 复现日志展示。
- 复现报告。

### 验收标准

- [ ] 能创建一个完整 case 目录。
- [ ] 能导入 `.brep`、`.step` 等输入数据。
- [x] 能运行最小 DRAW 脚本并捕获输出。
- [x] 能通过 result JSON 表达 DRAW 成功 / 失败状态。
- [ ] UI 能将状态写回当前 Case。
- [ ] 能生成真实 Case 的 `repro_report.md`。

---

## 12. P4：几何查看与 Shape 检查

### 目标

实现 OCCT 几何查看和基础 Shape 检查能力，使开发者可以直接在工作台中观察输入模型、异常边/面、中间结果和修复结果。

### 当前状态

状态：**OCCT Viewer 已支持 BREP/STEP/IGES 导入与加载，Viewer 已改为懒加载以降低启动阶段 native viewport 风险，checkshape smoke 已完成；选择、高亮和 Shape 基础统计已接入，跨拓扑永久命名仍需推进**。

### 主要任务

- [x] 集成 OCCT Viewer 到 Qt 界面基础。
- [x] 支持导入并加载 BREP。
- [x] 支持加载 STEP / IGES。
- [ ] 支持基础显示：shaded、wireframe、透明、边线。
- [ ] 支持选择 face / edge / vertex。
- [ ] 支持显示 subshape ID。
- [ ] 支持高亮异常 edge / face。
- [x] 实现 Shape 基础统计：Viewer 的 `topologySummary()` 已输出 Vertices、Edges、Wires、Faces、Shells、Solids 等统计，并同步到几何检查表与 Case manifest。
- [x] 实现最小 checkshape CTest 和结构化解析基础。
- [ ] 实现基础 checkshape 结果在 UI 中展示。
- [ ] 实现 shape dump 导出。
- [ ] 实现截图保存。
- [ ] 实现 before / after 对比入口。

### UI 面板

#### 几何视图

- 3D 视口。
- 视角控制工具栏。
- 选择模式切换。
- 高亮对象列表。
- 坐标轴指示。
- 截图按钮。

#### 几何检查面板

- Shape 总体状态。
- 拓扑数量统计。
- 异常类型统计。
- 异常子对象列表。
- 重新检查按钮。
- 导出报告按钮。

### 交付物

- 可用的 OCCT Qt Viewer。
- Shape 信息面板。
- 几何检查结果展示。
- 异常边/面高亮。

### 验收标准

- [x] 能加载并显示 BREP 模型。
- [x] 能加载并显示小型 STEP / IGES 模型。
- [ ] 能选择并高亮边/面。
- [ ] 能显示 Shape 拓扑统计。
- [ ] 能将异常对象与日志中的 ID 关联。
- [ ] 能保存视图截图。

---

## 13. P5：源码定位与证据链

### 目标

将运行日志、调用栈、源码文件、几何对象和复现脚本串联成可审查的证据链。

### 当前状态

状态：**Evidence 面板、源码索引、DRAW 日志解析、源码关键词跳转、EvidenceBundle 结构化落盘和最小 Evidence UI 联动已有基础；调用栈精确跳转、日志行号跳转和真实几何对象高亮仍未完成**。

### 主要任务

- [x] 实现源码文件浏览器基础。
- [x] 实现只读代码查看基础能力。
- [x] 支持关键词搜索和跳转到命中文件行。
- [ ] 支持从调用栈跳转源码文件。
- [ ] 支持从日志错误跳转证据项。
- [ ] 实现调用栈解析。
- [x] 实现 DRAW 运行日志结构化解析基础。
- [x] 实现 Evidence 面板骨架。
- [x] 补齐 Evidence 数据模型并落盘为 `artifacts/evidence_bundle.json`。
- [ ] 支持证据项与几何对象关联。
- [ ] 支持证据项与源码文件关联。
- [ ] 生成 `diagnosis_evidence.json`。

### 证据类型

- 崩溃调用栈。
- DRAW 运行日志。
- C++ 复现日志。
- ASan 报告。
- Shape check 结果。
- Shape 基础统计已接入；跨拓扑重建永久命名仍待增强。
- 几何截图。
- 输入输出模型差异。
- 相似历史问题。
- 相关源码文件。

### 交付物

- 源码查看器。
- 调用栈解析器。
- 日志解析器。
- 证据链面板。
- 证据导出文件。

### 验收标准

- [ ] 日志中的异常能进入真实 Case 证据链。
- [ ] 调用栈能显示并跳转源码。
- [ ] 证据项能关联 case、run、source、shape。
- [x] 能生成结构化证据文件。

---

## 14. P6：诊断结论与相似案例

### 目标

在证据链基础上生成明确、可审查的诊断结论，并支持检索相似案例、源码位置和历史问题。

### 当前状态

状态：**相似案例检索、源码本地搜索和诊断报告导出已有基础；候选根因结构和知识库索引仍未完成**。

### 主要任务

- [x] 实现诊断结论卡片基础。
- [ ] 实现候选根因数据结构。
- [ ] 实现置信度展示。
- [x] 实现相关源码搜索结果列表基础。
- [x] 实现相似案例检索基础。
- [ ] 实现本地知识库索引。
- [x] 支持基础本地搜索骨架。
- [x] 支持关键词搜索源码的基础模块。
- [ ] 支持按 toolkit / package / class / function 组织检索结果。
- [x] 生成 `diagnosis_report.md`。

### 相似案例来源

- 当前企业 case 库。
- `doc/` 设计文档与技术文档。
- OCCT 源码注释。
- 已归档补丁。
- 已归档测试结果。
- 手动导入的 issue 摘要。

### 交付物

- 诊断结论卡片。
- 相似案例面板。
- 本地搜索索引。
- 诊断报告。

### 验收标准

- [ ] 能基于 evidence 生成至少一个候选根因。
- [ ] 候选根因包含证据、相关源码、风险说明。
- [ ] 能检索历史 case。
- [ ] 能生成 `diagnosis_report.md`。

---

## 15. P7：补丁方案与代码审查

### 目标

实现候选补丁管理、diff 查看、风险标记和人工审查流程。

### 当前状态

状态：**候选补丁 diff、审查状态、patch apply/undo、候选 diff 导入导出、worktree diff 生成和验证签核门禁已完成最小闭环；自动源码修复生成、交互式冲突修复和自动提交/合并尚未实现**。

### 主要任务

- [x] 实现 PatchCandidate / PatchReview 基础。
- [x] 实现补丁方案卡片骨架。
- [x] 实现 diff viewer。
- [ ] 实现补丁文件列表。
- [ ] 实现风险等级标记。
- [ ] 实现影响模块标记。
- [ ] 支持人工编辑 patch 说明。
- [x] 支持应用 patch 到工作区。
- [x] 支持撤销 patch。
- [x] 支持从 `.patch/.diff` 导入候选补丁。
- [x] 支持从配置的 `patch.worktree_root` 执行 `git diff --binary HEAD` 生成候选补丁。
- [x] 支持保存候选补丁并写出带 sha256 的 manifest。
- [x] 支持验证签核门禁，把 PatchCandidate 与 VerificationReport 绑定。
- [ ] 支持生成回归测试草案。
- [x] 支持生成 patch review 报告。

### 补丁审查对话框

字段与区域：

- Patch 摘要。
- 修改文件列表。
- Diff 内容。
- 风险等级。
- 影响模块。
- 是否修改公开 API。
- 是否新增测试。
- 是否建议提交 upstream。
- 人工审查备注。

按钮：

- 应用补丁。
- 撤销补丁。
- 标记可验证。
- 退回修改。
- 导出 patch。

### 交付物

- 候选补丁查看与管理。
- diff 显示。
- patch 应用 / 撤销。
- patch 审查报告。

### 验收标准

- [x] 能显示补丁 diff。
- [x] 能将 patch 应用到指定 OCCT worktree。
- [x] 能撤销 patch。
- [x] 能记录人工审查状态。
- [x] 能导出 `.patch` 文件。
- [x] 能在验证报告通过且审查门禁可接受时签核；失败时阻塞并记录原因。

---

## 16. P8：回归验证与测试报告

### 目标

实现从原始问题复验、相关测试运行、testgrid 结果解析到验证报告生成的完整验证能力。

### 当前状态

状态：**CTest DRAW 门禁、testgrid/testdiff 解析骨架、UI 最小 runner 和 before/after 最小结构化对比已完成；`testgrid_plan` 支持 root/executable/arguments/group/grid/case，完整失败明细、耗时和自动两阶段 testgrid/testdiff 编排仍未完成**。

### 主要任务

- [x] 实现底部 testgrid 面板的最小验证计划输入。
- [ ] 支持运行原始 repro。
- [ ] 支持运行新增测试。
- [x] 支持配置并运行指定 testgrid group / grid / case 的最小命令。
- [x] 支持运行 DRAW smoke CTest。
- [x] 建立 `draw_smoke` 作为后续 testgrid 前置门禁。
- [x] 支持解析 testgrid/testdiff 输出骨架。
- [x] 底部 testgrid 面板可运行最小门禁并写入 `artifacts/testgrid_result.json`。
- [x] 支持解析测试通过率基础字段。
- [x] 未配置真实 testgrid executable 时降级为门禁和 summary 解析，不伪造完整回归。
- [ ] 支持解析完整失败列表、耗时。
- [ ] 支持 before / after 结果对比。
- [ ] 支持性能变化记录。
- [x] 支持生成最小 `verification_report.md` 与 `verification/verification_report.json`。
- [ ] 支持将验证结果绑定到 patch candidate。

### 验证计划对话框

字段：

- 构建类型：Debug / RelWithDebInfo / Release / ASan。
- 验证范围：原始 repro / 新增测试 / 相关测试 / 全量测试。
- testgrid group。
- testgrid grid。
- 超时时间。
- 并行任务数。
- 是否保存截图。
- 是否做性能对比。

按钮：

- 保存计划。
- 开始验证。
- 停止验证。
- 查看报告。

### 验证等级

```text
V0 编译通过
V1 原始问题 patch 后成功
V2 新增测试通过
V3 相关测试组通过
V4 testgrid/testdiff 无明显退化
V5 企业内部历史 case 通过
V6 全量回归通过
```

### 交付物

- 验证计划。
- testgrid runner。
- 测试结果面板。
- 验证报告。

### 验收标准

- [ ] 能运行原始 repro 并比较 patch 前后结果。
- [ ] 能运行指定测试集合。
- [x] 有解析通过率的基础模块。
- [ ] UI 能展示通过率。
- [ ] 能列出失败用例。
- [x] 能生成最小 `verification_report.md`。

---

## 17. P9：知识归档与案例复用

### 目标

将已处理 case 的复现、诊断、补丁、验证结果沉淀为企业内部知识资产，支持后续检索、复用和升级验证。

### 主要任务

- [ ] 实现归档对话框。
- [ ] 归档 case manifest。
- [ ] 归档输入数据摘要。
- [ ] 归档复现脚本。
- [ ] 归档证据链。
- [ ] 归档 patch。
- [ ] 归档验证报告。
- [ ] 生成知识条目。
- [ ] 实现标签体系。
- [ ] 实现历史案例搜索。
- [ ] 支持从历史 case 复制复现模板。
- [ ] 支持重复问题识别。

### 知识条目标签

```yaml
tags:
  algorithm: "Fillet"
  toolkit: "TKFillet"
  package: "BRepFilletAPI"
  failure_mode: "Null curve"
  shape_type: "BREP"
  reproducible: true
  patch_status: "verified"
  upstream_candidate: true
```

### 归档对话框

字段：

- 问题摘要。
- 根因摘要。
- 影响模块。
- 解决方式。
- 适用版本。
- 是否可复用。
- 是否可公开。
- 标签。
- 备注。

按钮：

- 归档入库。
- 仅保存本地。
- 导出报告包。
- 取消。

### 交付物

- 企业知识库基础能力。
- 归档流程。
- 历史案例检索。
- 重复问题识别。

### 验收标准

- [ ] 已完成 case 能归档。
- [ ] 归档后能通过关键词检索。
- [ ] 能查看历史复现脚本、patch 和验证报告。
- [ ] 能从历史 case 创建新 case。

---

## 18. P10：产品化、插件化与稳定性增强

### 目标

完善工具的可维护性、扩展性、性能和稳定性，使其成为团队可长期使用的内部工程工具。

### 主要任务

- [ ] 插件化 runner。
- [ ] 插件化诊断规则。
- [ ] 插件化报告模板。
- [ ] 完善权限与保密策略。
- [ ] 完善 UI 布局保存。
- [ ] 完善错误恢复机制。
- [ ] 完善任务取消机制。
- [ ] 支持大型 case 的异步加载。
- [ ] 支持日志压缩与清理。
- [ ] 支持 crash recovery。
- [ ] 支持导入 / 导出 case pack。
- [ ] 支持应用自更新或版本检查。
- [ ] 支持企业 CI 集成。

### 交付物

- 稳定版本包。
- 插件接口说明。
- 管理员配置说明。
- 用户手册。
- 内部部署指南。

### 验收标准

- [ ] 连续处理多个 case 不丢数据。
- [ ] 大模型或大日志加载不阻塞主界面。
- [ ] runner 异常不会导致主程序崩溃。
- [ ] case pack 可导入导出。
- [ ] 用户可以恢复上次工作现场。

---

## 19. 菜单体系实施清单

### 19.1 顶部菜单

```text
文件
  新建问题...
  打开 Case...
  导入 Case Pack...
  导出 Case Pack...
  保存当前状态
  最近打开
  退出

视图
  案例列表
  流程状态
  源码视图
  几何视图
  证据链
  诊断面板
  补丁面板
  验证面板
  底部控制台
  重置布局

复现
  生成 DRAW 复现脚本
  生成 C++ 最小复现
  运行当前复现
  停止运行
  保存复现快照
  打开复现目录

分析
  采集证据
  解析调用栈
  运行 Shape 检查
  检索相似案例
  生成诊断报告

补丁
  新建补丁候选
  应用补丁
  撤销补丁
  查看 Diff
  导出 Patch
  标记待验证

验证
  新建验证计划
  运行原始复现
  运行相关测试
  运行 testgrid
  对比结果
  生成验证报告

知识库
  归档当前 Case
  搜索历史案例
  从历史案例创建
  管理标签
  导出知识条目

工具
  环境配置...
  检查 OCCT 环境
  打开 DRAW 控制台
  打开 PowerShell
  清理临时文件
  设置...

帮助
  查看文档
  快捷键
  关于
```

### 19.2 主工具栏按钮

```text
问题录入
复现生成
源码分析
补丁方案
回归验证
知识归档
```

### 19.3 常用右键菜单

#### Case 列表右键菜单

```text
打开 Case
复制 Case
重命名
标记状态
打开目录
导出 Case Pack
归档
删除
```

#### 源码编辑器右键菜单

```text
跳转定义
查找引用
复制路径
复制函数名
添加到证据链
标记为候选修改点
打开文件所在目录
```

#### 几何视图右键菜单

```text
选择 Vertex
选择 Edge
选择 Face
显示 ID
高亮对象
添加到证据链
导出子 Shape
保存截图
重置视角
```

#### 证据项右键菜单

```text
打开来源
关联源码
关联几何对象
设为关键证据
从诊断中移除
导出证据
```

#### 补丁 Diff 右键菜单

```text
应用此文件修改
忽略此文件修改
复制 Diff
打开源码
添加审查备注
标记风险
```

---

## 20. 对话框实施清单

### 20.1 新建问题对话框

用途：创建标准 case。

必要能力：

- 输入问题描述。
- 选择问题类型。
- 导入模型和代码。
- 填写期望与实际结果。
- 设置保密级别。
- 自动生成 case ID。

### 20.2 环境配置对话框

用途：配置 OCCT、Qt、CMake、VS、workspace 路径。

必要能力：

- 自动检测。
- 手工选择路径。
- 验证 DRAWEXE。
- 验证 CMake。
- 保存本地配置。

### 20.3 复现生成向导

用途：从输入文件和操作步骤生成 DRAW / C++ 复现。

步骤：

1. 选择输入模型。
2. 选择复现方式。
3. 填写操作命令。
4. 选择输出对象。
5. 生成脚本。
6. 运行验证。

### 20.4 证据采集对话框

用途：选择要采集的证据类型。

选项：

- 运行日志。
- 调用栈。
- Shape check。
- Shape dump。
- 截图。
- ASan 报告。
- testgrid 结果。

### 20.5 补丁审查对话框

用途：人工审查候选补丁。

必要能力：

- 查看 diff。
- 查看影响文件。
- 设置风险等级。
- 填写审查意见。
- 应用或退回补丁。

### 20.6 验证计划对话框

用途：配置并启动验证。

必要能力：

- 选择构建类型。
- 选择测试范围。
- 设置并行任务数。
- 设置超时。
- 启动 / 停止验证。

### 20.7 归档对话框

用途：将解决后的 case 入库。

必要能力：

- 编辑问题摘要。
- 编辑根因摘要。
- 设置标签。
- 设置公开等级。
- 导出报告包。
- 入库。

### 20.8 设置对话框

用途：统一配置工具行为。

分类：

- 通用。
- 路径。
- 构建。
- 复现。
- 验证。
- 知识库。
- 安全与保密。
- UI 外观。

---

## 21. 模块接口规划

### 21.1 Runner 接口

```cpp
class IRunner
{
public:
    virtual ~IRunner() = default;
    virtual RunnerResult run(const RunnerRequest& request) = 0;
    virtual void cancel(const QString& runId) = 0;
};
```

典型实现：

- `DrawRunner`
- `CMakeRunner`
- `CTestRunner`
- `TestgridRunner`
- `PowerShellRunner`

### 21.2 Case 服务接口

```cpp
class CaseService
{
public:
    CaseId createCase(const CaseCreateRequest& request);
    CaseManifest loadCase(const CaseId& id);
    void saveCase(const CaseManifest& manifest);
    void updateStatus(const CaseId& id, CaseStatus status);
    QList<CaseSummary> listCases(const CaseQuery& query);
};
```

### 21.3 Evidence 服务接口

```cpp
class EvidenceService
{
public:
    EvidenceId addEvidence(const CaseId& caseId, const EvidenceRecord& record);
    QList<EvidenceRecord> listEvidence(const CaseId& caseId);
    void linkToSource(const EvidenceId& evidenceId, const SourceLocation& location);
    void linkToShape(const EvidenceId& evidenceId, const ShapeReference& shapeRef);
};
```

### 21.4 Verification 服务接口

```cpp
class VerificationService
{
public:
    VerificationRunId createPlan(const CaseId& caseId, const VerificationPlan& plan);
    VerificationResult runPlan(const VerificationRunId& id);
    VerificationSummary summarize(const VerificationRunId& id);
};
```

---

## 22. UI 与功能映射

| UI 区域 | 后端模块 | 初始阶段 | 最终能力 |
|---|---|---:|---|
| 案例列表 | case | P1 | Case 管理、状态筛选、归档 |
| 流程状态 | case / workflow | P1 | 状态机、步骤跳转、任务记录 |
| 关键输入 | case | P1 | 输入模型、复现方式、保密级别 |
| 源码视图 | source | P5 | 调用栈跳转、候选修改点 |
| 复现脚本 | repro | P3 | DRAW/C++ 脚本生成与运行 |
| 几何视图 | geometry | P4 | Shape 显示、选择、高亮、截图 |
| 几何检查 | geometry | P4 | checkshape、拓扑统计、异常定位 |
| 证据摘要 | diagnosis | P5 | 证据链、关键证据标记 |
| 诊断结论 | diagnosis | P6 | 根因假设、置信度、源码关联 |
| 候选补丁 | patch | P7 | diff、风险、应用、撤销 |
| 验证结果 | verify | P8 | testgrid、性能、回归报告 |
| 相似案例 | knowledge | P6/P9 | 历史 case、issue、文档检索 |
| DRAW 控制台 | runner | P3 | 运行脚本、捕获输出 |
| CMake 控制台 | runner | P3/P8 | 构建、ctest、日志 |
| testgrid 结果 | verify | P8 | 测试统计、失败详情 |

---

## 23. MVP 范围建议

### MVP 必须包含

- [ ] Qt 主界面完整骨架。
- [ ] Case 创建、打开、保存。
- [ ] 本地环境配置与检测。
- [ ] DRAW 脚本编辑与运行。
- [ ] 运行日志捕获。
- [ ] OCCT 几何模型加载与显示。
- [x] Shape 基础统计。
- [ ] 调用栈 / 日志手动录入与展示。
- [ ] 证据链面板。
- [ ] 诊断报告 Markdown 导出。

### MVP 可暂缓

- [ ] 自动补丁生成。
- [ ] 自动最小化模型。
- [ ] ASan 深度集成。
- [ ] 全量 testgrid 自动化。
- [ ] 复杂相似 issue 在线检索。
- [ ] 企业级权限系统。
- [ ] 插件市场式扩展。

### MVP 验收场景

使用一个简单 Fillet/Boolean/STEP 问题 case，完成：

1. 创建 case。
2. 配置 OCCT 环境。
3. 导入模型。
4. 编写 DRAW 复现脚本。
5. 运行 DRAW。
6. 捕获失败日志。
7. 在几何视图中显示模型。
8. 添加关键证据。
9. 编写诊断结论。
10. 导出报告。

---

## 24. 优先实现的样板 Case

建议第一个完整样板采用以下类型：

```text
问题类型：Fillet / Chamfer 建模失败
输入数据：BREP 或 STEP 转 BREP 后的最小模型
复现方式：DRAW Tcl
失败模式：Null curve / invalid p-curve / small radius failure
验证方式：原始 DRAW 复现 + 新增本地测试脚本
```

样板 case 目录：

```text
cases/OCC-LOCAL-2026-0001/
  input/valve_body_min.brep
  repro/repro.tcl
  logs/draw.stdout.log
  logs/draw.stderr.log
  artifacts/repro.png
  artifacts/shape_before.brep
  report/repro_report.md
  report/diagnosis_report.md
  report/verification_report.md
```

该样板需要贯穿以下模块：

- Case Explorer。
- Workflow Stepper。
- DRAW Runner。
- Geometry Viewer。
- Shape Info。
- Evidence Panel。
- Diagnosis Card。
- Verification Summary。
- Report Export。

---

## 25. 状态机设计

### 25.1 Case 状态

```text
new             新建
prepared        已准备输入
env_checked     环境已采集
to_reproduce    待复现
reproduced      已复现
minimized       已最小化
evidence_ready  证据已收集
diagnosing      分析中
diagnosed       已诊断
patching        补丁处理中
patch_ready     补丁待验证
verifying       验证中
verified        已验证
archived        已归档
blocked         阻塞
```

### 25.2 状态流转

```text
new
  → prepared
  → env_checked
  → to_reproduce
  → reproduced
  → minimized
  → evidence_ready
  → diagnosing
  → diagnosed
  → patching
  → patch_ready
  → verifying
  → verified
  → archived
```

### 25.3 异常状态

```text
blocked_missing_data       缺少输入数据
blocked_env_error          环境错误
blocked_not_reproduced     无法复现
blocked_build_failed       构建失败
blocked_test_failed        验证失败
blocked_need_review        需要人工审查
```

---

## 26. 报告体系

### 26.1 复现报告

`report/repro_report.md`

内容：

- 问题摘要。
- 环境信息。
- 输入文件。
- 复现脚本。
- 运行命令。
- 运行结果。
- 日志摘要。
- 截图 / dump / shape 文件。
- 是否稳定复现。

### 26.2 诊断报告

`report/diagnosis_report.md`

内容：

- 问题现象。
- 证据列表。
- 调用栈。
- 几何检查结果。
- 相关源码文件。
- 候选根因。
- 置信度。
- 风险说明。
- 建议补丁方向。

### 26.3 验证报告

`report/verification_report.md`

内容：

- Patch 信息。
- 构建信息。
- 原始复现结果。
- 新增测试结果。
- 相关 testgrid 结果。
- 性能变化。
- 失败用例。
- 是否通过验收。

### 26.4 归档报告

`report/archive_summary.md`

内容：

- 问题一句话摘要。
- 根因一句话摘要。
- 解决方式。
- 适用版本。
- 标签。
- 复用建议。
- 是否建议 upstream。

---

## 27. 开发质量要求

### 27.1 代码质量

- C++ 代码保持清晰模块边界。
- UI 与业务逻辑分离。
- runner 与 UI 解耦。
- 长任务不得阻塞 UI 线程。
- 所有外部命令必须记录 command、working directory、environment、exit code。
- 所有文件路径应支持中文路径与空格路径。
- 所有 case 文件应使用 UTF-8。

### 27.2 错误处理

必须处理：

- OCCT 路径错误。
- DRAWEXE 不存在。
- DLL 缺失。
- 模型文件无法读取。
- 脚本运行超时。
- 外部进程崩溃。
- 构建失败。
- 测试失败。
- 文件被占用。
- workspace 权限不足。

### 27.3 日志要求

日志至少包括：

- 应用日志。
- case 操作日志。
- runner 日志。
- DRAW stdout / stderr。
- CMake stdout / stderr。
- testgrid 输出。
- 用户关键操作记录。

---

## 28. 安全与保密设计

### 28.1 保密等级

```text
public        可公开
internal      企业内部
confidential 机密
restricted   严格受限
```

### 28.2 保密策略

- 默认所有 case 为 `internal`。
- 机密 case 禁止导出完整模型。
- 严格受限 case 禁止复制到公共 workspace。
- 报告导出时可选择是否包含模型路径、hash、截图。
- 导出 case pack 前必须弹出保密确认。

### 28.3 UI 提示

- case 卡片显示保密标记。
- 导出按钮显示风险提示。
- 对外提交前显示检查清单。

---

## 29. 测试策略

### 29.1 单元测试

覆盖：

- 配置解析。
- case manifest 读写。
- 状态机流转。
- runner command 构造。
- 日志解析。
- evidence 数据模型。
- report 生成。

### 29.2 集成测试

覆盖：

- 创建 case。
- 导入模型。
- 运行 DRAW。
- 捕获日志。
- 加载 Shape。
- 生成报告。
- 执行验证计划。

### 29.3 UI 测试

覆盖：

- 主界面启动。
- 菜单动作。
- 对话框字段校验。
- 布局保存恢复。
- case 切换。
- 任务运行进度展示。

### 29.4 样板数据测试

至少准备：

```text
sample_cases/
  fillet_null_curve/
  boolean_invalid_result/
  step_import_missing_color/
  shape_healing_invalid_edge/
```

---

## 30. 风险清单与应对

| 风险 | 表现 | 应对 |
|---|---|---|
| OCCT 环境复杂 | DRAWEXE 无法启动、DLL 缺失 | 环境检测和明确错误提示先行 |
| UI 阻塞 | 运行脚本时主界面卡死 | 所有外部任务使用异步 worker |
| 模型数据过大 | 加载慢、内存高 | 延迟加载、显示简化、后台统计 |
| 日志格式不稳定 | 解析失败 | 保留原始日志，结构化解析可失败降级 |
| 自动补丁误判 | patch 风险高 | 补丁默认人工审查，不自动合并 |
| 回归测试耗时 | 开发效率下降 | 分级验证，先跑局部，再跑完整 |
| 企业数据泄露 | 导出误带模型 | 保密等级、导出检查、脱敏选项 |
| 目录中文路径 | 外部工具执行失败 | 路径统一加引号，测试中文路径 |

---

## 31. 版本交付清单

### R0：技术验证版

当前状态：**基本完成，剩余配置系统与文档收口**。

- [x] Qt Widgets 应用启动。
- [x] 新工作台第一屏启动。
- [x] DRAW smoke CTest 通过。
- [x] 最小 checkshape smoke CTest 通过。
- [x] 旧代码清理完成。
- [x] 环境采集脚本可输出 JSON。
- [x] DRAWEXE 可由脚本/CTest 调用。
- [x] DRAWEXE 可由应用入口调用基础。
- [x] OCCT Viewer 最小嵌入 demo。
- [ ] 配置文件可加载。
- [ ] 环境信息可在 UI 展示真实采集结果。

### R1：工作台基础版

- [x] 主界面完整。
- [ ] 主界面拆分为可维护模块。
- [ ] Case workspace 创建/加载/保存可用。
- [x] 环境采集脚本可用。
- [ ] 环境采集接入 UI 与 Case。
- [x] DRAW 复现运行基础可用。
- [ ] DRAW 复现结果写入 Case artifacts。
- [ ] 日志可展示并按 Case 落盘。

### R2：复现与几何版

- [x] 模型导入。
- [x] 几何显示基础嵌入。
- [x] Shape 基础统计。
- [x] Markdown 报告生成骨架。
- [ ] 真实 Case 复现报告生成。

### R3：证据与诊断版

- [ ] 调用栈解析。
- [ ] 证据链管理。
- [ ] 源码跳转。
- [ ] 诊断报告生成。

### R4：补丁与验证版

- [x] diff 查看。
- [ ] patch 应用 / 撤销。
- [x] patch 审查状态与报告。
- [x] 验证计划基础。
- [x] testgrid/testdiff 结果解析骨架。
- [x] testgrid/testdiff 最小 runner 与 UI 接入。
- [ ] 验证报告生成。

### R5：知识闭环版

- [ ] case 归档。
- [ ] 历史案例搜索。
- [ ] 从历史 case 复用。
- [ ] case pack 导入导出。

## 32. Definition of Done

一个功能只有满足以下条件，才视为完成：

- [ ] UI 可操作。
- [ ] 后端逻辑可运行。
- [ ] 错误场景有提示。
- [ ] 日志完整记录。
- [ ] 数据落盘可恢复。
- [ ] 至少有一个样板 case 验证。
- [ ] 文档已更新。
- [ ] 不阻塞主线程。
- [ ] 不破坏已有 case 数据格式。

一个 case 只有满足以下条件，才视为已闭环：

- [ ] 有明确问题描述。
- [ ] 有环境快照。
- [ ] 有输入数据或可替代样板数据。
- [ ] 有可执行复现。
- [ ] 有运行日志。
- [ ] 有证据链。
- [ ] 有诊断结论。
- [ ] 有补丁或处理建议。
- [ ] 有验证结果。
- [ ] 有归档记录。

---

## 33. 最终开发计划

基于当前仓库状态，后续开发应按“真实 Case 闭环优先、自动化增强后置”的原则推进。不要再做旧功能迁移，也不要继续扩大静态 mock；每个阶段都必须能构建、能运行、能验证。

### 33.1 M1：Case 工作台最小闭环

```text
1. 统一本地配置和 CMake preset
2. 建立 AppContext + ConfigService
3. 建立 cases/<case_id>/ workspace 标准目录
4. 补齐 CaseManifest / WorkflowState / WorkspaceLayout
5. 创建 sample case 并支持打开/保存
6. 将 WorkbenchWindow 静态数据替换为 sample case 数据绑定
7. 环境采集写入 case/env/env_snapshot.json
8. DRAW UI 运行结果写入 case/logs 与 case/artifacts
9. DRAW 日志解析进入 Evidence 面板
10. 生成第一份真实 case/report/report.md
11. 导出失败 DRAW Repro Pack
```

M1 完成后，项目从“可运行骨架”进入“真实 Case 工作台”。这是当前最重要的里程碑。

### 33.2 M2：几何与证据闭环

```text
1. OCCT Viewer 支持加载小型 BREP/STEP/IGES
2. Shape 基础统计
3. checkshape 结果 UI 展示
4. EvidenceBundle 落盘为 JSON
5. 日志行、Evidence 项、源码位置建立跳转关系
6. 生成 diagnosis_evidence.json
7. 报告引用 Evidence 与几何检查结果
```

M2 完成后，工作台可以支撑真实 OCCT 问题的复现和证据审查。

### 33.3 M3：验证与回归闭环

```text
1. CTest runner 接入 UI
2. testgrid/testdiff runner 接入 UI
3. 所有 testgrid/testdiff 任务依赖 draw_smoke / draw_ready 门禁
4. 解析通过率、失败列表和日志路径
5. 生成 verification_report.md
6. 将验证结果绑定到 PatchCandidate
```

M3 完成后，候选补丁才具备基本工程验证条件。

### 33.4 M4：诊断、补丁与知识闭环

```text
1. 源码索引与调用栈跳转
2. 相似案例和历史知识检索
3. 候选根因结构化表达
4. PatchCandidate diff 查看和人工审查
5. patch 应用/撤销
6. verification_report.md 与 PatchCandidate 绑定
7. case 归档与知识条目生成
8. 从历史 case 复用 repro 和验证模板
```

M4 完成后，工具形成从问题到知识沉淀的完整工程闭环。

### 33.5 开发节奏约束

- 每个 M 阶段拆成 1-3 天可验收的小任务。
- 每个任务必须更新相关文档和验收清单。
- 每次新增外部命令执行都必须有日志、退出码和失败提示。
- 每次新增 Case artifact 都必须记录相对路径和生成来源。
- 不在没有稳定复现的情况下推进自动补丁。

## 34. 首个演示场景脚本

### 场景名称

```text
Fillet Null Curve 问题复现与诊断
```

### 演示步骤

1. 打开工具。
2. 新建 case：`OCC-LOCAL-2026-0001`。
3. 导入 `valve_body_min.brep`。
4. 选择问题类型：`Modeling / Fillet`。
5. 生成 DRAW 复现脚本。
6. 运行 DRAW。
7. 控制台显示 `Null curve` 错误。
8. 几何视图高亮异常边 `E125`。
9. 证据链添加：运行日志、Shape 信息、调用栈、异常边。
10. 诊断面板显示候选根因。
11. 导出诊断报告。
12. 归档为历史案例。

### 演示成功标准

- [ ] 全流程在一个主界面中完成。
- [ ] 无需手动切换到外部命令行。
- [ ] 复现日志、几何视图、证据链、诊断结论能联动展示。
- [ ] 报告能在 case 目录中生成。

---

## 35. 后续增强方向

### 35.1 自动最小化数据

- 子 Shape 裁剪。
- STEP assembly 裁剪。
- Boolean 参数最小化。
- Fillet edge 集合最小化。

### 35.2 更强源码智能分析

- libclang / clangd 索引。
- 函数调用图。
- toolkit 依赖图。
- Git blame / diff 辅助。

### 35.3 更强验证能力

- testdiff 集成。
- 截图差异比较。
- 性能基线对比。
- 企业历史 case 批量回归。

### 35.4 团队协作能力

- 多人审查。
- case 分配。
- 审批流。
- 内部 issue 系统对接。

### 35.5 AI 辅助能力

- 根据日志生成初步诊断。
- 根据 DRAW 失败生成复现说明。
- 根据 evidence 生成报告草稿。
- 根据源码上下文生成候选 patch 草案。
- 根据测试结果生成风险总结。

AI 辅助能力必须遵循以下原则：

```text
AI 可以建议，但不能绕过编译、复现、测试和人工审查。
```

---

## 36. 当前立即可执行任务

当前环境已经具备 OCCT、Qt、DRAWEXE 和最小 CTest 验证。下一步立即任务应围绕 M1：Case 工作台最小闭环推进，而不是继续扩展静态 UI 或提前进入自动补丁。

### 36.1 N1：配置与上下文

- [x] 清理/统一 `CMakePresets.json`，确保团队共享 preset 不依赖本机 `CMakeUserPresets.json`。
- [x] 明确 `src/QtWorkbenchDefaults.cmake` 为本地 Qt kit 配置入口，并保持 gitignore。
- [x] 增加 `config/workbench.default.yaml`。
- [x] 增加 `config/workbench.local.example.yaml`。
- [x] 更新 `.gitignore`，覆盖 `config/workbench.local.yaml`、`cases/`、`artifacts/`、`knowledge/cache/` 等本地产物。
- [x] 实现 `AppContext`。
- [x] 实现 `ConfigService`，读取默认配置、本地配置和 CMake 推导路径。

说明：当前 `workbench.*.yaml` 文件采用 JSON-compatible YAML 子集，避免在 N1 阶段引入新第三方依赖。后续若确实需要完整 YAML 语法，再增加解析器或转换层。

### 36.2 N2：Case workspace

- [x] 定义 `WorkflowState`、`WorkspaceLayout`，并补齐 `CaseManifest` 与 workflow/layout 的关系。
- [x] 创建第一个 sample case 目录结构。
- [x] 支持 sample case workspace 初始化。
- [x] 支持 `CaseManifest` 保存/加载。
- [x] 支持 UI 创建、打开、保存 case。
- [x] 将 `WorkbenchWindow` 初始数据从 sample case workspace 加载，并支持从 `cases/` 扫描与切换多个 Case。
- [x] 将左侧 Case/流程/关键输入 UI 拆分为 `CasePanel`，主窗口仅保留 Case workspace 行为编排。
- [x] DRAW、环境采集等新增 artifact 使用相对路径索引；旧 mock 字段仍需逐步迁移。

### 36.3 N18：CasePanel 拆分

- [x] 新增 `workbench/CasePanel.*`。
- [x] CasePanel 只负责左侧 UI 展示和发出新建/打开/保存/刷新/切换信号。
- [x] Case workspace 扫描、manifest 保存/加载和错误提示仍由 `WorkbenchWindow` 编排，避免在 UI widget 内混入业务逻辑。
- [x] 继续拆分证据、源码等面板。

### 36.4 N19：WorkspaceLayout 恢复

- [x] 主 splitter 左/中/右宽度可保存到 `workspace_layout.left_width` / `center_width` / `right_width`。
- [x] 中心 tab 使用稳定 ID 保存/恢复：`source`、`repro`、`geometry`、`evidence`、`diff`、`environment`。
- [x] 底部 tab 使用稳定 ID 保存/恢复：`draw`、`cmake`、`testgrid`。
- [x] 底部控制台高度继续保存到 `workspace_layout.bottom_height`。
- [ ] 窗口位置、最大化状态和更细粒度 dock/layout 状态后续再补。

### 36.5 N20：Workbench 面板拆分收束

- [x] 新增 `workbench/VerificationPanel.*`。
- [x] VerificationPanel 负责验证指标展示。
- [x] VerificationPanel 通过信号触发 Markdown 报告和 Repro Pack 导出。
- [x] testgrid 结果落盘后通过 `VerificationPanel::setItems()` 刷新验证指标。
- [x] 新增 `workbench/EvidencePanel.*`，负责证据摘要、证据表刷新和追加展示。
- [x] 新增 `workbench/SourcePanel.*`，负责源码搜索输入、源码文本显示、搜索结果展示和结果激活信号。
- [x] `WorkbenchWindow` 保留源码索引、文件读取、Case workspace 和报告编排，避免 UI widget 混入业务持久化。

### 36.6 N3：DRAW 到 Case

- [x] UI 运行 DRAW 后将 stdout/stderr/result JSON 写入当前 Case。
- [x] 使用等价 C++ 基础解析生成 DRAW Evidence。
- [x] 在 Evidence 面板展示 success token、错误行、checkshape 状态摘要，并生成 `draw_log_analysis.json`。
- [x] 支持从 UI 导出 Repro Pack。

### 36.7 N21：Patch Apply / Undo 最小入口

- [x] `config/workbench.default.yaml` 与 `config/workbench.local.example.yaml` 增加可选 `patch.worktree_root`。
- [x] `CaseManifest` 增加 `patch.worktree_root`、`patch.apply_status`、`patch.apply_log`。
- [x] Patch Review 面板增加 Apply / Undo 按钮。
- [x] Apply 执行 `git apply artifacts/candidate_patch.diff`，Undo 执行 `git apply -R artifacts/candidate_patch.diff`。
- [x] 执行结果写入 `logs/patch_apply.*` / `logs/patch_undo.*` 与 `artifacts/patch_apply_result.json` / `artifacts/patch_undo_result.json`。
- [x] 执行结果登记为 Evidence，并同步到验证指标。
- [ ] 当前不自动生成真实 patch、不处理交互式冲突、不自动提交，也不绕过 reviewer 审查状态。

### 36.8 N22：EvidenceBundle 结构化落盘

- [x] 新增 `core/evidence/EvidenceBundleWriter.*`。
- [x] 保存 Case 时生成 `artifacts/evidence_bundle.json`。
- [x] EvidenceBundle 包含 evidence records、source/log/geometry/artifact 分类、几何检查、验证指标、诊断摘要和 patch 状态。
- [x] EvidenceBundle 链接状态只保存 Case 相对路径，不写入本机绝对路径。
- [x] Markdown 报告在结构化证据包存在时链接 `artifacts/evidence_bundle.json`。
- [x] 新增 `evidence_bundle_smoke` CTest，验证 sample case 可生成结构化证据包且包含 source/log 证据分类。
- [x] 证据项与源码、日志、几何视图的最小 UI 联动跳转已接入；调用栈、日志行号和真实几何对象级定位仍待后续补齐。

### 36.9 N4：环境与报告

- [x] UI 触发 `verify_env.ps1` 并保存 `env_snapshot.json`。
- [x] 在环境信息 tab 显示 Qt、OCCT、CMake、MSVC、DRAW smoke 状态。
- [x] 生成当前 Case Markdown 报告，输出到 `case/report/repro_report.md`。
- [x] 报告中避免泄露不必要的个人绝对路径；Evidence artifact 仅接受 Case workspace 内相对路径。
- [x] 报告生成器会检查 Evidence artifact 链接，区分 `ok`、`missing artifact` 和 `blocked absolute/external path`。

### 36.10 N23：VerificationReport 结构化落盘

- [x] 新增 `core/verify/VerificationReportWriter.*`。
- [x] 保存 Case 时生成 `report/verification_report.md` 与 `verification/verification_report.json`。
- [x] VerificationReport 汇总 DRAW gate、testgrid/testdiff、patch apply 状态、EvidenceBundle 链接和 overall gate。
- [x] `repro_report.md` 在验证报告存在时链接 `report/verification_report.md`。
- [x] 新增 `verification_report_smoke` CTest，验证 sample case 可生成结构化验证报告且不写入本机绝对路径。
- [x] `failure_details` 与 patch dry-run gate 已有最小结构化输出。
- [x] before/after 最小结构化对比和 patch conflict hints 已在 N27 补齐。
- [ ] 耗时、完整自动两阶段 testgrid/testdiff 编排和交互式冲突修复仍待后续补齐。

### 36.11 N24：Evidence UI 联动跳转

- [x] `EvidencePanel` 增加行激活信号并保存当前 Evidence records。
- [x] `file:line` 证据引用切换到源码 tab，优先打开仓库/Case 内可解析文件；找不到文件时显示 Case manifest 中的源码摘要。
- [x] `logs/` 与普通 Case artifact 证据只按 Case 相对路径解析，日志显示到底部 DRAW/CMake 控制台。
- [x] Shape/Geometry 证据切换到几何 tab，并选中失败或告警几何检查项。
- [x] 调用栈帧、日志行号和几何对象级定位已在 N26 补齐基础。
- [ ] 真实 OCCT shape/edge/face 高亮仍待 Viewer 层增强。

### 36.12 N25：Patch dry-run 与验证失败详情

- [x] Patch Apply 前执行 `git apply --check artifacts/candidate_patch.diff` dry-run。
- [x] Patch Undo 前执行 `git apply -R --check artifacts/candidate_patch.diff` dry-run。
- [x] dry-run 失败时不修改目标 worktree，并写入 `logs/patch_*_dry_run.*` 与 `artifacts/patch_*_dry_run_result.json`。
- [x] dry-run 结果登记为 Evidence，并同步到 verification items、EvidenceBundle 和 VerificationReport。
- [x] VerificationReport 新增 `patch_dry_run` gate、`failure_details` 和 patch dry-run artifact 链接。
- [x] `verification_report_smoke` 覆盖 `patch_dry_run` gate 与 `failure_details` 字段。
- [ ] dry-run 当前为 UI 线程同步执行；后续应改为异步任务并支持取消。
- [x] patch dry-run conflict hints 与 testgrid before/after 最小结构化对比已在 N27 补齐。
- [ ] 完整交互式冲突修复和自动两阶段 testgrid/testdiff 编排仍待后续补齐。

### 36.13 N26：Evidence 精确定位

- [x] `EvidenceRecord` 增加可选定位字段：`source_file/source_line`、`log_file/log_line`、`stack_frame`、`geometry_object`。
- [x] Case manifest JSON 读写兼容旧 evidence，并保留新增定位字段。
- [x] Evidence 激活优先使用结构化字段；兼容 `file:line` 与 `logs/x.log:line`。
- [x] 源码证据可跳转到源码 tab 的目标行；调用栈帧可解析 `file:line`。
- [x] 日志证据可打开底部 DRAW/CMake 控制台并滚动到目标行。
- [x] 几何证据可按 `geometry_object` 匹配几何检查行，匹配不到时退回失败/告警项。
- [x] DRAW 运行证据、patch dry-run/apply 证据会记录可跳转日志行。
- [x] EvidenceBundle 输出 `location` 对象，`evidence_bundle_smoke` 覆盖 source/log/geometry 定位字段。
- [x] OCCT Viewer 子形状高亮与 Evidence 选择同步基础已在 N28 补齐。
- [x] 鼠标拾取反向同步和截图证据基础已在 N31 补齐。
- [ ] 跨拓扑重建的稳定命名或 shape dump 映射仍待后续增强。

### 36.14 N27：Patch/testgrid before-after 与冲突摘要

- [x] 新增 `TestgridComparison` / `TestgridComparisonRow`，可按模块对齐 before/after testgrid 行。
- [x] 支持读取 `verification/testgrid_before.txt` 和 `verification/testgrid_after.txt`，生成 pass/fail delta、回归状态和摘要。
- [x] UI 运行 testgrid 后会把 before/after 对比写入 `artifacts/testgrid_result.json.before_after`，并刷新差异对比 tab 摘要。
- [x] VerificationReport 新增 `before_after` gate、JSON 区块和 Markdown “Before / After Comparison” 区块。
- [x] before/after 出现新增失败或退化时会进入 `failure_details` 并影响 overall gate。
- [x] EvidenceBundle 新增 `verification_comparison` 区块。
- [x] Patch dry-run 失败时会解析 stderr/stdout 中的 conflict hints，写入 `artifacts/patch_*_dry_run_result.json.conflicts`。
- [x] `evidence_bundle_smoke` 与 `verification_report_smoke` 覆盖 before/after 对比字段。
- [ ] 当前仍不是自动执行 patch 前/patch 后两轮 testgrid；before/after 文件或运行结果需要由用户/后续 runner 提供。
- [ ] 当前 conflict hints 只是结构化摘要，还没有交互式冲突修复 UI。

### 36.15 N28：OCCT Viewer 子形状高亮与 Evidence 选择同步

- [x] `OcctViewerWidget` 保存最近显示的 `TopoDS_Shape`，并跟踪当前高亮对象。
- [x] 新增 `highlightGeometryObject()` / `clearHighlight()` / `highlightedObjectId()`。
- [x] 支持解析 `V/E/W/F/SHELL/SOLID + 序号` 的 `geometry_object`。
- [x] 通过 `TopExp_Explorer` 在当前 shape 中查找真实子形状，使用单独 `AIS_Shape` 着色和加粗显示。
- [x] Evidence 激活时会同步几何 tab、几何检查表和 Viewer 高亮状态。
- [x] 找不到对象、对象 ID 不支持或当前未加载 shape 时，会在几何摘要和控制台显示明确失败原因。
- [x] Viewer 鼠标拾取反向同步和截图证据基础已在 N31 补齐。
- [ ] 当前子形状序号依赖 OCCT 遍历顺序；后续需要引入跨拓扑重建的稳定命名或 shape dump 映射。

### 36.16 N29：VerificationReport 与 PatchCandidate 审查结论绑定

- [x] VerificationReport 新增 `gate.patch_review`，把 PatchCandidate 审查状态纳入验证门禁。
- [x] VerificationReport JSON 新增 `patch.decision`，包含审查状态、门禁状态、推荐动作、阻塞原因和审查项。
- [x] Markdown 验证报告新增 Patch State 审查门禁、推荐动作、结论摘要和审查项表格。
- [x] 审查通过但 testgrid、DRAW 或 before/after 等验证门禁仍失败时，`patch_review` 会进入 `blocked`，并把 `candidate_patch` 写入 `failure_details`。
- [x] 审查拒绝时可让 overall gate 失败；审查仍为 draft/pending 且其他验证已通过时，overall gate 保持 incomplete。
- [x] `patch_review.md` 会链接 `verification_report.md` 和 `verification_report.json`，审查报告导出后同步刷新 VerificationReport。
- [x] EvidenceBundle 已输出 patch 审查状态和审查项，便于后续归档和知识检索。
- [x] `verification_report_smoke` 与 `evidence_bundle_smoke` 覆盖 patch review 结构化输出。
- [ ] 尚未实现自动签核、真实补丁生成或完整 testgrid/testdiff 回归签核。

### 36.17 N30：自动两阶段 testgrid/testdiff 编排

- [x] 底部 testgrid 面板新增“二阶段验证”入口。
- [x] 新增最小异步状态机，执行顺序为 before DRAW gate / testgrid command -> patch dry-run/apply -> after DRAW gate / testgrid command -> patch undo。
- [x] before 和 after 阶段分别写入 `logs/testgrid_before_*`、`logs/testgrid_after_*`、`artifacts/testgrid_before_result.json`、`artifacts/testgrid_after_result.json`。
- [x] before 和 after 阶段会写入 `verification/testgrid_before.txt` 与 `verification/testgrid_after.txt`，供现有 before/after parser、VerificationReport 和 EvidenceBundle 复用。
- [x] 最终结果写入 `artifacts/testgrid_two_stage_result.json`，并同步兼容入口 `artifacts/testgrid_result.json`。
- [x] 最终 JSON 包含 `mode=two_stage`、phase artifact、patch 状态、testgrid rows、testdiff entries 和 before/after comparison。
- [x] 未配置真实 testgrid executable 时，不伪造大型回归，只使用 DRAW gate 和本地 summary 文件形成可审查的最小结果。
- [x] patch worktree 或候选 diff 不可用时，workflow 会以 blocked 状态结束并保留 before 阶段证据。
- [x] 二阶段状态机已在 N33 抽成可独立 smoke 测试的 `VerificationWorkflow` 服务。
- [ ] 真实 testdiff before/after 工件、完整失败明细、耗时统计和交互式冲突修复仍待后续增强。

### 36.18 N31：OCCT Viewer 反向拾取、稳定拓扑标识与截图证据

- [x] `OcctViewerWidget` 新增 `geometryObjectPicked(objectId, summary)` 信号。
- [x] Viewer 对显示的 `AIS_Shape` 启用 vertex、edge、face、solid、compsolid、compound 子形状选择模式。
- [x] 鼠标左键拾取命中子形状后，会映射为标准 `geometry_object`：`V/E/W/F/SHELL/SOLID/COMPSOLID/COMPOUND/SHAPE + 序号`。
- [x] 拾取后会复用 `highlightGeometryObject()` 高亮当前对象，并把选择摘要回传工作台。
- [x] 工作台收到 Viewer 拾取后，会生成 `artifacts/geometry_selection_*.json`，登记 Geometry Evidence，并同步几何检查表和 EvidenceBundle。
- [x] 几何 tab 新增“保存截图”按钮，当前 Viewer 画面保存为 `artifacts/geometry_screenshot_*.png`。
- [x] 截图会登记为 Geometry Evidence；若当前有高亮对象，则 Evidence 同时记录 `geometry_object`。
- [x] Viewer 提供 `topologySummary()`，把当前 shape 的 V/E/W/F/SHELL/SOLID/COMPSOLID/COMPOUND 计数写入选择和截图证据摘要。
- [ ] 当前所谓稳定标识仍是当前 shape 内的 `TopExp_Explorer` 遍历序号，只能稳定复用同一模型/同一加载结果；跨修复前后拓扑重建仍需 shape dump 映射或命名算法。
- [ ] 截图当前使用 Qt `grab()`，后续可升级为 OCCT `V3d_View` 原生导出以获得更可控的分辨率和背景。

### 36.19 N32：补丁候选生成、导入导出与验证签核

- [x] 候选补丁 diff 面板支持手工编辑，不再只是只读展示。
- [x] 支持从 `.patch/.diff` 文件导入候选补丁，并保存到当前 Case 的 `artifacts/candidate_patch.diff`。
- [x] 支持从配置的 `patch.worktree_root` 执行 `git diff --binary HEAD` 生成候选补丁，不写死个人机器路径。
- [x] 保存候选补丁时写出 `artifacts/candidate_patch_manifest.json`，记录 case id、相对 patch 路径、bytes、sha256、review/signoff 状态和 worktree 配置状态。
- [x] 支持从 UI 导出当前候选 diff 为 `.patch/.diff` 文件。
- [x] 候选 diff 变化时会重置 patch review、patch apply 状态和 signoff 状态，避免旧审查结论误用于新补丁。
- [x] Patch signoff 会读取最新 `verification/verification_report.json`，只有 `overall_status=passed` 且 `gate.patch_review` 可接受时写入 `signed off`。
- [x] 验证未通过、审查未通过或验证报告缺失时，Patch signoff 写入 `blocked` 并记录阻塞原因。
- [x] VerificationReport 新增 `gate.patch_signoff`、`patch.signoff_status` 和 `patch.signoff_note`，Markdown 报告同步展示签核状态。
- [x] EvidenceBundle 的 patch 摘要包含 signoff 状态和说明，便于后续归档与知识检索。
- [x] `verification_report_smoke` 与 `evidence_bundle_smoke` 覆盖 patch signoff 结构化输出。
- [ ] 当前仍不自动生成源码修复，不自动提交/合并，也不提供交互式冲突修复 UI。
- [ ] `git diff` 生成当前为同步执行；后续可改为异步 CommandRunner 并记录独立 stdout/stderr/result artifact。

### 36.20 N33：二阶段验证状态机服务化

- [x] 新增 `core/verify/VerificationWorkflow.*`，集中描述二阶段验证的业务状态和下一动作决策。
- [x] `VerificationWorkflow` 覆盖 before gate、before command、patch apply、after gate、after command、patch undo 和 finalize 阶段。
- [x] `WorkbenchWindow` 不再直接持有 before/after command result、patch applied、final status 等二阶段业务状态，只保留命令启动、artifact 落盘和 UI 刷新编排。
- [x] 服务 decision 支持“先持久化 phase，再启动下一动作或 finalize”，避免失败路径丢失阶段证据。
- [x] 新增 `verification_workflow_smoke` CTest，覆盖无配置 testgrid happy path、配置 testgrid command path、DRAW gate 失败和 patch apply 失败。
- [x] 全量 CTest 当前包含 `draw_smoke`、`draw_checkshape_smoke`、`evidence_bundle_smoke`、`verification_report_smoke` 和 `verification_workflow_smoke`。
- [ ] 当前服务只做状态决策，不直接执行命令、不写文件；这是有意边界，命令和 artifact 仍由工作台编排层负责。
- [x] two-stage final result JSON 写入已由 N56 抽到独立 writer；后续继续降低 `WorkbenchWindow` 的 UI 刷新与报告触发职责。

### 36.21 N34：拓扑签名与 shape dump 映射

- [x] 新增 `core/geometry/TopologySignature.*`，把当前 shape 映射为可落盘的 topology signature JSON。
- [x] 每条记录包含 `object_id`、`stable_id`、类型、序号、orientation、children 和 `brep_sha256`。
- [x] `brep_sha256` 基于 `BRepTools::Write(subshape)` 的 SHA-256，而不是进程内 TShape 指针 hash。
- [x] Viewer 暴露 `topologySignatureForObject()` 和 `topologySignatureJson()`，拾取对象时 summary 中带 `stable_id`。
- [x] 几何 tab 新增“保存映射”入口，手动写出 `artifacts/topology_signature_*.json`。
- [x] 加载模型、Viewer 拾取和保存截图时会写出 topology signature artifact，并登记 Geometry Evidence。
- [x] `geometry_selection_*.json` 新增 `stable_geometry_object`、`topology_signature` 和新的 selection basis。
- [x] 新增 `topology_signature_smoke` CTest，覆盖签名结构、对象签名、`stable_id` helper 和缺失对象错误路径。
- [x] 新增 `TopologySignature::compare()`，可对 before/after signature JSON 执行同类型匹配，优先 exact `brep_sha256`，再用 `object_id`、orientation、children 和 index 生成近似分数。
- [x] compare 结果包含 `matches`、`unmatched_before`、`unmatched_after`、`counts_delta` 和 summary，便于后续落盘为几何差异 artifact。
- [x] `topology_signature_smoke` 已覆盖相同 shape 的稳定匹配和尺寸变化 shape 的非稳定匹配。
- [ ] 当前 `stable_id` 和 compare 策略能帮助审查和 before/after 对比，但仍不是可保证跨所有拓扑重建的永久命名。
- [ ] 后续需要继续增强局部几何、邻接关系和容差近似匹配，并把 compare artifact 接入几何差异 tab、EvidenceBundle 与 VerificationReport。

### 36.22 N35：testgrid/testdiff 失败明细、耗时统计与工件归档

- [x] `VerificationResultParser` 新增结构化失败明细与耗时摘要模型，可从 testgrid rows、testdiff entries 和 before/after comparison 生成统一 failure details。
- [x] 单阶段 `artifacts/testgrid_result.json` 新增 `failure_details`、`timing` 和 `testdiff_artifacts`，记录 DRAW gate、配置 testgrid 命令、testdiff summary 和命令日志。
- [x] 二阶段 `artifacts/testgrid_before_result.json` / `testgrid_after_result.json` / `testgrid_two_stage_result.json` 同步输出失败明细、耗时和 testdiff 工件；兼容入口 `artifacts/testgrid_result.json` 保持可被报告与证据包读取。
- [x] `VerificationReportWriter` 优先消费 `artifacts/testgrid_result.json` 中的结构化 N35 字段；Markdown 报告新增 Timing Summary，并在 Linked Artifacts 中列出 testdiff summary/stdout/stderr。
- [x] `EvidenceBundleWriter` 新增 `verification_failures`、`verification_timing` 和 `testdiff_artifacts`，把 testgrid/testdiff 失败、耗时和工件纳入证据包。
- [x] sample case 新增最小 `artifacts/testgrid_result.json`，用于覆盖 N35 字段且不包含本机绝对路径。
- [x] 新增 `verification_result_parser_smoke` CTest；`verification_report_smoke` 与 `evidence_bundle_smoke` 已覆盖 N35 输出字段。
- [ ] 当前 testdiff 工件仍以 summary/stdout/stderr 指针为主；后续接入真实 testdiff 目录时，应归档图片 diff、属性 diff、性能 diff 等细粒度文件清单。

### 36.23 N36：异步 patch git diff 生成与独立 artifact

- [x] PatchCandidate 的 `git diff --binary HEAD` 生成已从同步等待改为异步 `CommandRunner`，避免阻塞 UI 线程。
- [x] 生成命令会记录 program、arguments、working_directory、stdout、stderr、exit_code、exit_status、elapsed_ms 和 success。
- [x] stdout/stderr 分别写入 `logs/patch_generate.stdout.log` / `logs/patch_generate.stderr.log`，结构化结果写入 `artifacts/patch_generate_result.json`。
- [x] 命令成功且 stdout 存在 diff 时，继续保存 `artifacts/candidate_patch.diff` 与 `artifacts/candidate_patch_manifest.json`；无 diff 时不覆盖已有候选 diff。
- [x] Evidence、VerificationReport 和 EvidenceBundle 已归档 patch generation 状态与 `patch_generate_result` artifact。
- [x] sample case 新增最小 `artifacts/patch_generate_result.json`，`verification_report_smoke` 与 `evidence_bundle_smoke` 覆盖 N36 输出字段。
- [ ] 当前仍未提供 patch 生成取消按钮、生成任务队列和交互式冲突修复 UI；后续应与 CommandRunner 取消能力、Patch 服务拆分一起推进。

### 36.24 N38：two-stage result JSON writer 抽离

- [x] 新增 `core/verify/TwoStageVerificationResultWriter.*`，负责生成两阶段 phase result 与 final result JSON。
- [x] `WorkbenchWindow::persistTwoStagePhase()` 已改为传入已解析 rows、testdiff、failure_details、timing 等结构化数据，由 writer 生成 `testgrid_before_result.json` / `testgrid_after_result.json` 内容。
- [x] `WorkbenchWindow::persistTwoStageWorkflowResult()` 已改为由 writer 生成 `testgrid_two_stage_result.json` 与兼容入口 `testgrid_result.json` 内容。
- [x] 新增 `two_stage_verification_result_writer_smoke` CTest，覆盖 phase/final JSON 的 rows、failure_details、timing、testdiff artifacts、plan、phase artifact 和 before_after 字段。
- [ ] 当前文件读写、Evidence 登记、verification items 更新和 UI 刷新仍在 `WorkbenchWindow`；后续应继续拆分为 testgrid artifact service 与 UI adapter。

### 36.25 N39：真实 testdiff before/after 目录与细粒度工件清单

- [x] 新增 `core/verify/TestdiffArtifactScanner.*`，从 Case workspace 扫描约定 testdiff 目录并生成 manifest。
- [x] 支持目录约定：`verification/testdiff/{before,after,diff}`、`verification/testdiff_before`、`verification/testdiff_after`、`verification/testdiff_diff`、`artifacts/testdiff/{before,after,diff}`、`artifacts/testdiff_before`、`artifacts/testdiff_after`、`artifacts/testdiff_diff`。
- [x] `testdiff_artifacts` 新增 `directories`、`artifact_files`、`artifact_counts` 和 `truncated`；工件按 image/property/performance/log/text/other 分类，并保留 before/after/diff role。
- [x] 单阶段 `testgrid_result.json` 和二阶段 writer 都复用该 scanner，旧字段 `summary`、`command_stdout`、`command_stderr` 保持兼容。
- [x] VerificationReport JSON、Markdown Linked Artifacts 和 EvidenceBundle 已保留并展示细粒度 testdiff 工件清单。
- [x] sample case 新增最小 `artifacts/testdiff/{before,after,diff}` 工件；`verification_report_smoke`、`evidence_bundle_smoke`、`two_stage_verification_result_writer_smoke` 覆盖 N39 字段。
- [ ] 当前只扫描/归档既有 testdiff 文件，不负责生成图片 diff、属性 diff 或性能 diff；真实 testdiff runner 输出目录适配仍待后续完成。

### 36.26 N40：topology before/after match artifact 接入

- [x] 新增 `core/geometry/TopologyCompareArtifact.*`，统一读取 `artifacts/topology_compare.json`。
- [x] 当不存在 compare artifact 但存在 before/after signature JSON 时，可通过 `TopologySignature::compare()` 在内存生成同构对比对象。
- [x] 几何差异 tab 会展示 topology compare 摘要和 artifact 路径。
- [x] EvidenceBundle 新增 `geometry_diff`，包含 available/status/summary、match summary、matches、unmatched before/after 和 artifact。
- [x] VerificationReport JSON 新增 `geometry_diff`；Markdown 新增 Geometry Diff 区块和 topology compare artifact 链接。
- [x] sample case 新增最小 `artifacts/topology_compare.json`；`verification_report_smoke`、`evidence_bundle_smoke` 覆盖 N40 字段。
- [ ] 当前仍不在 Viewer 中提供 before/after signature 选择和 compare artifact 生成交互；真实跨拓扑重建永久命名仍待后续增强。

### 36.27 N41：testgrid artifact 文件服务抽离

- [x] 新增 `core/verify/TestgridArtifactService.*`，集中管理 Case workspace 下 `logs/`、`verification/`、`artifacts/` 的 testgrid 相关路径。
- [x] 服务支持目录创建、命令 stdout/stderr 写入、phase summary 写入/读取和 JSON artifact 写入。
- [x] `WorkbenchWindow::persistTestgridResult()` 已改用服务写入 gate/command 日志、读取 summary、写入 `artifacts/testgrid_result.json`。
- [x] `persistTwoStagePhase()` 已改用服务写入 phase 日志、读取 summary/testdiff、写入 `artifacts/testgrid_before_result.json` / `testgrid_after_result.json`。
- [x] `persistTwoStageWorkflowResult()` 已改用服务读取 before/after summary 并写入 `testgrid_two_stage_result.json` 与兼容入口 `testgrid_result.json`。
- [x] 新增 `testgrid_artifact_service_smoke` CTest，覆盖路径约定、目录创建、日志写入、phase summary 解析和 JSON artifact 写入。
- [ ] 当前只拆出文件/路径服务；Evidence 编排、verification item 更新和 testgrid UI 刷新仍在 `WorkbenchWindow`，后续应继续拆为服务/adapter。

### 36.28 N42：testdiff runner 输出目录适配器

- [x] 新增 `core/verify/TestdiffRunnerAdapter.*`，识别外部 runner 输出中的 `before`、`after`、`diff`、`delta` 或 `testdiff/{before,after,diff}` 目录。
- [x] adapter 将输出复制归一化到当前 Case workspace 的 `artifacts/testdiff/{before,after,diff}`。
- [x] adapter 复用 `TestdiffArtifactScanner` 生成 `directories`、`artifact_files`、`artifact_counts` 和 `truncated`。
- [x] manifest 新增 `adapter` 元数据，记录 source layout、复制文件数和导入目录；不记录 runner 输出绝对路径。
- [x] 新增 `testdiff_runner_adapter_smoke` CTest，覆盖 before/after/diff 导入、image/property/performance 分类和绝对路径不泄露。
- [ ] adapter 已接入 testdiff command 配置和 UI `Run testdiff` 入口；当前仍未实现图片/属性/性能 diff 生成算法，只导入并索引外部 runner 已生成工件。

### 36.29 N43：topology compare 生成入口与几何对象跳转

- [x] `TopologyCompareArtifact` 新增从 before/after signature JSON 构造 compare 对象的入口。
- [x] `TopologyCompareArtifact` 可把 compare 结果写入当前 Case 的 `artifacts/topology_compare.json`，并只记录 Case 相对路径或文件名，避免泄露外部 signature 绝对路径。
- [x] 几何差异 tab 新增“Generate topology compare”入口，可选择 before/after topology signature JSON 并生成 compare artifact。
- [x] 生成成功后刷新几何差异摘要、更新 verification metric、登记 Geometry Evidence，并同步写出 EvidenceBundle 与 VerificationReport。
- [x] 生成 Evidence 会优先关联当前 Viewer 高亮对象；无高亮对象时回退到 compare 中的 unmatched/matched 对象，形成几何对象级跳转线索。
- [x] 新增 `topology_compare_artifact_smoke` CTest，覆盖 artifact 写入、读取、摘要和本机绝对路径脱敏。
- [ ] 当前入口仍基于用户选择已落盘 signature JSON；尚未实现 before/after 模型加载会话管理、跨 Case compare 或永久命名增强。

### 36.30 N44：Evidence 与 testgrid UI presenter 拆分

- [x] 新增 `workbench/EvidenceCoordinator.*`，集中处理 EvidenceRecord 追加到 `WorkbenchMockData.evidenceItems` 和 `CaseManifest.evidenceItems` 的同步。
- [x] `WorkbenchWindow::appendEvidenceRecord()` 已改为委托 `EvidenceCoordinator`，窗口层不再手写 Evidence 数据/manifest 双写。
- [x] 新增 `workbench/TestgridTablePresenter.*`，把 testgrid rows 转为稳定的 5 列表格单元，并负责刷新 `QTableWidget`。
- [x] `WorkbenchWindow` 初始化、Case 切换刷新、单阶段 testgrid 结果刷新和二阶段结果刷新已复用 `TestgridTablePresenter`，消除重复填表逻辑。
- [x] 新增 `workbench_presenter_smoke` CTest，覆盖 Evidence 数据同步和 testgrid rows 到表格单元的映射。
- [ ] 当前仍只是工作台层 presenter/coordinator 拆分；单阶段 testgrid result artifact 与 verification item 更新已由 N47 抽出，二阶段 final 状态同步已由 N50 抽出，phase 解析/文件写入已由 N53 抽出，final JSON 写入已由 N56 抽出，final 后控件刷新已由 N59 抽出；diff artifact 表刷新和实际报告写出仍在 `WorkbenchWindow`。

### 36.31 N45：testdiff adapter 命令执行与 UI 接入

- [x] `VerificationPlan` 新增 `testdiff_arguments` 和 `testdiff_output_root`，并支持 Case manifest 读写。
- [x] `ConfigService`、默认配置、本地配置模板和 sample case 已同步新增 testdiff 参数与输出目录配置。
- [x] 应用启动时会把全局配置中的 testdiff executable/arguments/output root 回填到当前 Case 的 verification plan。
- [x] 底部 testgrid 面板新增 `Run testdiff` 入口；该入口先执行 `draw_smoke` 门禁，门禁通过后再运行配置的 testdiff 命令。
- [x] testdiff 命令支持 `{group}`、`{grid}`、`{case}`、`{workspace}`、`{verification}`、`{artifacts}` 和 `{output}` 占位符。
- [x] 命令 stdout/stderr 写入 `logs/testdiff_runner.stdout.log` / `logs/testdiff_runner.stderr.log`，命令文本写入 `verification/testdiff_summary.txt`。
- [x] `TestdiffRunnerAdapter` 会把 runner 输出目录中的 before/after/diff 工件导入 `artifacts/testdiff/{before,after,diff}`，并写出 `artifacts/testdiff_adapter_result.json` 和 `artifacts/testdiff_adapter_manifest.json`。
- [x] 兼容入口 `artifacts/testgrid_result.json` 会更新 `testdiff_entries`、`testdiff_artifacts` 和 adapter 状态，使现有 VerificationReport 与 EvidenceBundle 继续消费真实 testdiff 工件。
- [x] 新增 `case_manifest_plan_smoke` CTest，覆盖 testdiff plan 新字段读写。
- [x] 本次因 `CaseManifest.h` 结构体扩展触发过旧对象 ABI 不一致，clean rebuild 后恢复；后续修改核心 manifest/model 结构后应至少执行一次 clean rebuild 或确认所有依赖目标已重编。
- [ ] 当前不生成图片 diff、属性 diff 或性能 diff，只导入外部 runner 已生成工件；testdiff 命令规划已由 N48 抽出，adapter result 写入与 UI/Evidence/报告触发同步已由 N51 抽出。

### 36.32 N46：topology signature 局部几何与容差近似增强

- [x] `TopologySignature` 记录已补充 `subshape_counts`，用于描述每个子对象下的 vertex/edge/wire/face/shell/solid 等局部拓扑规模。
- [x] `TopologySignature` 记录已补充 `geometry`，包含 bounding box、bbox center、bbox diagonal、线/面/体 measure 和 center of mass。
- [x] before/after 近似匹配评分新增局部几何和子拓扑统计项，在策略提示中输出 `same_measure`、`same_bbox_diagonal`、`same_bbox_center`、`same_subshape_counts` 等可审查依据。
- [x] `topology_signature_smoke` 已覆盖新增字段与非 exact hash 场景下的增强匹配提示。
- [x] `topology_compare_artifact_smoke` 继续覆盖 compare artifact 写入和脱敏路径，确认增强签名不破坏几何差异工件。
- [ ] 当前增强仍是启发式拓扑/几何近似匹配，不是完整永久命名算法；真实跨布尔、倒角、重建链路仍需要后续结合建模历史、局部邻接图和更强的几何容差策略。

### 36.33 N47：单阶段 testgrid result writer 服务化

- [x] 新增 `core/verify/TestgridResultWriter.*`，集中负责单阶段 testgrid result 的解析、verification items、diff summary、failure details、timing 和 `artifacts/testgrid_result.json` 组装。
- [x] `TestgridResultWriter::writeSingleStageResult()` 会写入 Case workspace 的兼容入口 `artifacts/testgrid_result.json`，继续供 VerificationReport 与 EvidenceBundle 消费。
- [x] `WorkbenchWindow::persistTestgridResult()` 已改为只写命令日志、调用 writer、同步当前 UI 数据、登记 Evidence、保存 Case 和触发报告。
- [x] 新增 `testgrid_result_writer_smoke` CTest，覆盖 command 输出解析、verification items、timing、before/after comparison、JSON 落盘和 Case 相对路径约定。
- [ ] 二阶段 final result 的状态同步和报告触发意图已由 N50 抽出，phase 解析/文件写入已由 N53 抽出，final JSON 写入已由 N56 抽出，实际 UI 控件刷新已由 N59 抽出；后续应继续抽取 diff artifact 表刷新和实际报告写出。

### 36.34 N48：testdiff command planner 服务化

- [x] 新增 `core/verify/TestdiffCommandPlanner.*`，集中处理 testdiff executable 校验、输出目录规范化/创建、工作目录 fallback、占位符替换和 `CommandRequest` 组装。
- [x] 支持 `{group}`、`{grid}`、`{case}`、`{workspace}`、`{verification}`、`{artifacts}` 和 `{output}` 占位符，保持 UI 既有配置语义。
- [x] `WorkbenchWindow::startConfiguredTestdiffCommand()` 已改为调用 planner，窗口层只保留启动 runner、记录最后输出目录和控制台提示。
- [x] 新增 `testdiff_command_planner_smoke` CTest，覆盖默认输出目录、相对输出目录、工作目录 fallback、占位符替换和缺少 executable 的错误路径。
- [x] testdiff adapter result JSON、manifest 合并、Evidence 登记、EvidenceBundle/VerificationReport 刷新已由 N51 拆为 result writer/coordinator。

### 36.35 N49：testdiff 图片/属性/性能工件索引策略

- [x] 新增 `core/verify/TestdiffArtifactIndex.*`，消费 `TestdiffArtifactScanner` 已发现的 `artifact_files`，不直接生成或伪造外部 diff 结果。
- [x] `artifact_index` 输出 `schema_version`、`supported_kinds`、按 kind/role 统计的 `counts`、配对后的 `groups` 和策略说明。
- [x] 当前索引支持 image/property/performance 三类关键 diff 工件，按 normalized relative name 将 before/after/diff 归并到同一组，并标记 `paired_with_diff`、`paired`、`diff_only` 或 `incomplete`。
- [x] `TestdiffArtifactScanner` 已在 manifest 中写入 `artifact_index`；`TestdiffRunnerAdapter` 导入 runner 输出后会自动获得该索引，不记录 runner 输出绝对路径。
- [x] 新增 `testdiff_artifact_index_smoke` CTest，覆盖 image/property/performance 计数、配对状态、unsupported log 忽略和策略字段；`testdiff_runner_adapter_smoke` 同步覆盖 manifest 集成。
- [ ] 当前仍不实现图片像素 diff、属性结构 diff 或性能趋势生成；VerificationReport/EvidenceBundle 展示 `artifact_index` 摘要已由 N52 完成，性能文本轻量解析已由 N54 完成，生成器边界已由 N57 固化。

### 36.36 N50：二阶段 final result 状态同步 coordinator

- [x] 新增 `workbench/TwoStageFinalResultCoordinator.*`，负责二阶段 final result 完成后同步 `WorkbenchMockData` 与 `CaseManifest` 中的 testgrid rows、diff summary 和 verification items。
- [x] coordinator 复用 `EvidenceCoordinator` 登记 `Two-stage testgrid verification` Evidence，并返回 save/report 触发意图，使窗口层不再手写 final Evidence 和 manifest 双写。
- [x] `WorkbenchWindow::persistTwoStageWorkflowResult()` 已改为调用 coordinator；窗口层保留控件刷新、Case 保存、EvidenceBundle/VerificationReport 实际写出和控制台提示，final JSON 写入已由 N56 抽出。
- [x] 二阶段 final 完成后会同时触发 `writeEvidenceBundle()` 与 `writeVerificationReport()`，避免新增 final Evidence 后结构化证据包落后于验证报告。
- [x] `workbench_presenter_smoke` 已覆盖二阶段 final 状态同步、manifest 同步、Evidence 追加和报告触发标志。
- [ ] 当前二阶段 phase result 的解析/文件写入已由 N53 抽出，final JSON 写入已由 N56 抽出，final UI adapter 已由 N59 抽出；后续可继续抽取 diff artifact 表刷新和报告写出边界。

### 36.37 N51：testdiff adapter result writer/coordinator

- [x] 新增 `core/verify/TestdiffAdapterResultWriter.*`，集中负责 testdiff runner stdout/stderr 日志、`verification/testdiff_summary.txt`、`artifacts/testdiff_adapter_result.json`、`artifacts/testdiff_adapter_manifest.json` 和兼容入口 `artifacts/testgrid_result.json` 写入。
- [x] writer 复用 `TestgridArtifactService`、`VerificationResultParser` 与 `TestdiffRunnerAdapter`，输出继续包含 `testdiff_entries`、`testdiff_artifacts` 和 `artifact_index`，并保持 Case 相对路径约定。
- [x] 新增 `workbench/TestdiffAdapterResultCoordinator.*`，负责把 writer result 同步到 `WorkbenchMockData`、`CaseManifest`、verification items 与 Evidence，并返回保存 Case、刷新 EvidenceBundle 和 VerificationReport 的触发意图。
- [x] `WorkbenchWindow::persistTestdiffAdapterResult()` 已改为调用 writer/coordinator；窗口层只保留控件刷新、Case 保存、报告实际写出和控制台提示。
- [x] 新增 `testdiff_adapter_result_writer_smoke` CTest，覆盖 adapter result、manifest、兼容 testgrid result、summary/log 写入、artifact_index 和 runner 输出路径脱敏。
- [x] `workbench_presenter_smoke` 已扩展覆盖 testdiff adapter result 的 UI 数据、manifest、Evidence 和报告触发同步。
- [ ] 当前仍不生成图片像素 diff 或属性结构 diff；性能文本轻量指标提取已由 N54 完成，但尚未建立性能趋势/基线对比。

### 36.38 N52：VerificationReport/EvidenceBundle 展示 artifact_index 摘要

- [x] `VerificationReportWriter` 已把 `testdiff_artifacts.artifact_index` 清洗后写入 `verification/verification_report.json`，并新增 `artifact_index_summary`，包含总 group 数、配对状态计数和 image/property/performance kind 组数。
- [x] Markdown 验证报告新增 `Testdiff Artifact Index` 小节，展示 indexed kind 的 before/after/diff/total/groups 计数，以及前 12 个工件组的 kind/key/status/before/after/diff。
- [x] `EvidenceBundleWriter` 已归档同样的 `artifact_index` 与 `artifact_index_summary`，供后续 UI、知识归档和 case pack 消费。
- [x] sample case 的 `artifacts/testgrid_result.json` 已补充当前 `artifact_index` 示例，和 scanner/adapter 最新输出契约保持一致。
- [x] `verification_report_smoke` 与 `evidence_bundle_smoke` 已覆盖 N52 JSON 摘要和 Markdown 小节。
- [ ] 当前仍只展示/归档外部 runner 已生成工件的索引；N54 已补充属性 JSON 与性能文本轻量解析，但不计算图片像素差异、不解析属性结构差异、不生成性能趋势。

### 36.39 N53：二阶段 phase result writer 服务化

- [x] 新增 `core/verify/TwoStagePhaseResultWriter.*`，集中负责二阶段 before/after phase 的 DRAW gate 日志、可选 testgrid command 日志、summary fallback 解析、phase summary 写入、failure_details/timing 构造和 `artifacts/testgrid_<phase>_result.json` 写入。
- [x] writer 复用 `TestgridArtifactService`、`VerificationResultParser` 和 `TwoStageVerificationResultWriter::buildPhaseResult()`，保持既有 phase JSON 契约和 `testdiff_artifacts.artifact_index` 输出能力。
- [x] `WorkbenchWindow::persistTwoStagePhase()` 已改为调用 writer；窗口层只保留 after 阶段 testgrid rows 同步和控制台提示。
- [x] 删除窗口层旧 `writeTestgridPhaseSummary()` 包装，避免 phase summary 写入路径重复。
- [x] 新增 `two_stage_phase_result_writer_smoke` CTest，覆盖 gate/command 日志、phase summary、phase JSON、failure/timing 和 testdiff artifact index。
- [ ] 当前二阶段 final JSON 写入已由 N56 抽出，UI adapter 已由 N59 抽出；EvidenceBundle/VerificationReport 实际刷新仍在 `WorkbenchWindow`，后续可继续拆 report trigger。

### 36.40 N54：testdiff artifact analysis 最小解析策略

- [x] 新增 `core/verify/TestdiffArtifactAnalysis.*`，消费 `artifact_index`，不直接扫描外部路径、不引入第三方依赖。
- [x] image 工件分析记录 before/after/diff 可用性、配对状态和策略说明，明确 `diff_supplied_by_runner`，不在本工具中生成像素 diff。
- [x] property 工件分析对已有 JSON 做最小解析，输出 JSON 类型、顶层 key 数和 key 摘要；非法 JSON 会给出错误摘要。
- [x] performance 工件分析从文本中提取简单数值指标，支持带名称的百分比/ms 等常见 runner 输出，也支持单值行。
- [x] `TestdiffArtifactScanner` 已在 `testdiff_artifacts` 中写入 `artifact_analysis`，因此 adapter、单阶段和二阶段 result 会自然携带该结构。
- [x] `VerificationReportWriter` 与 `EvidenceBundleWriter` 已归档 `artifact_analysis`；Markdown 验证报告新增 `Testdiff Artifact Analysis` 小节。
- [x] sample case 的 `artifacts/testgrid_result.json` 已补充 `artifact_analysis` 示例。
- [x] 新增 `testdiff_artifact_analysis_smoke`，并扩展 `testdiff_runner_adapter_smoke`、`testdiff_adapter_result_writer_smoke`、`verification_report_smoke`、`evidence_bundle_smoke` 覆盖该路径。
- [x] 当前仍不生成真实图片像素 diff、不做属性结构 diff、不维护性能趋势基线；N57 已将该边界固化为结构化 generation policy，后续若引入真实生成器，还需定义 runner 输入/输出契约和 artifact 敏感信息脱敏边界。

### 36.41 N55：artifact_index / artifact_analysis 差异视图

- [x] 新增 `workbench/DiffArtifactsPresenter.*`，将 `testdiff_artifacts.artifact_index.groups` 转为差异 tab 的 kind/key/status/before/after/diff 表格行。
- [x] presenter 同时将 `artifact_analysis.groups` 转为 kind/key/status/analysis 表格行，image/property/performance 分别展示 runner diff 来源、属性 JSON 摘要和性能指标摘要。
- [x] 差异对比 tab 新增 artifact index 表和 artifact analysis 表，继续保留 topology compare 入口和原 diff summary。
- [x] Case 切换、topology compare、单阶段 testgrid、testdiff adapter 和二阶段 final result 更新后会刷新差异表。
- [x] `workbench_presenter_smoke` 已覆盖 N55 行模型，避免 UI 表格契约退化。
- [x] artifact 打开、kind/status 过滤和独立 DiffPanel 已由 N58/N61 补齐；后续仍可增强图片内嵌预览、路径复制和更细的工件搜索。

### 36.42 N56：二阶段 final result writer 服务化

- [x] 新增 `core/verify/TwoStageFinalResultWriter.*`，复用 `TwoStageVerificationResultWriter::buildWorkflowResult()` 和 `TestgridArtifactService` 写入 final JSON。
- [x] writer 同时写入 `artifacts/testgrid_two_stage_result.json` 与兼容入口 `artifacts/testgrid_result.json`，保持 VerificationReport、EvidenceBundle 和差异 tab 的既有消费路径。
- [x] `WorkbenchWindow::persistTwoStageWorkflowResult()` 已改为先调用 writer 写出最新 final artifact，再调用 `TwoStageFinalResultCoordinator` 同步 UI 数据、Evidence 和报告触发意图。
- [x] 由于 final artifact 先写入，N55 差异表刷新会读取最新 `artifact_index` / `artifact_analysis`，避免显示上一轮 testgrid result。
- [x] `two_stage_verification_result_writer_smoke` 已扩展覆盖 final writer 的实际落盘和兼容入口内容。
- [x] 当前 final 后 `VerificationPanel`、`EvidencePanel`、testgrid 表和保存/报告触发动作已由 N59 抽为 UI adapter；diff artifact 表刷新和 EvidenceBundle/VerificationReport 实际写出仍由 `WorkbenchWindow` 调用。

### 36.43 N57：testdiff 真实生成器边界策略

- [x] 新增 `core/verify/TestdiffGenerationPolicy.*`，把图片像素 diff、属性结构 diff 和性能趋势 diff 的生成器状态统一表达为 boundary-only policy。
- [x] policy 会记录每类生成器的 `enabled=false`、`generation_performed=false`、候选输入、当前输入计数、阻塞原因和后续契约，不生成任何图片、属性结构 diff、性能趋势 artifact。
- [x] `TestdiffArtifactAnalysis::build()` 已在 `artifact_analysis.generation_policy` 中携带该策略，因此 scanner、adapter、单阶段/二阶段 result、VerificationReport 和 EvidenceBundle 可以沿既有路径归档该边界。
- [x] sample case 的 `artifacts/testgrid_result.json` 已补充 `generation_policy` 示例，保持演示数据与当前 schema 一致。
- [x] 新增 `testdiff_generation_policy_smoke`，并扩展 `testdiff_artifact_analysis_smoke`，分别覆盖策略服务本身和 artifact analysis 集成路径。
- [x] 当前仍不启用真实生成器；N60 已先定义 opt-in 配置入口、输出命名和脱敏边界，后续实现具体像素/结构/趋势算法前仍需补齐容差/阈值策略和失败报告格式。

### 36.44 N58：差异视图 artifact 打开/跳转与过滤

- [x] `DiffArtifactsPresenter` 已支持可选 kind/status 过滤，并继续输出稳定的 index/analysis 表格行。
- [x] presenter 已兼容 N54 当前 `artifact_analysis` schema：image 使用 `before_supplied/after_supplied`，property 使用 `analysis.json`，同时保留旧字段兼容。
- [x] 差异对比 tab 新增 kind/status 下拉过滤和 `Open artifact` 入口；索引表双击可打开当前行 artifact，分析表双击可跳转到对应索引工件。
- [x] artifact 打开逻辑限制在当前 Case workspace 内，拒绝绝对路径、URL 和越界相对路径；文本、日志、JSON、diff/patch 等在底部控制台预览，图片等二进制工件走系统关联程序。
- [x] `workbench_presenter_smoke` 已覆盖过滤行模型和当前 analysis schema 摘要。
- [x] 差异页已由 N61 拆为独立 `DiffPanel`；后续可增加图片内嵌预览、路径复制和更细的搜索/过滤。

### 36.45 N59：二阶段 final UI adapter 与报告触发边界

- [x] 新增 `workbench/TwoStageFinalResultUiAdapter.*`，将二阶段 final 后控件刷新从 `WorkbenchWindow::persistTwoStageWorkflowResult()` 的手写块抽出。
- [x] UI adapter 负责刷新 diff label、testgrid 表、VerificationPanel 和 EvidencePanel，并返回 `refreshDiffArtifacts`、`saveCaseManifest`、`writeEvidenceBundle`、`writeVerificationReport` 动作。
- [x] `WorkbenchWindow` 仍负责 final writer、coordinator 调用、diff artifact 表刷新、Case 保存和 EvidenceBundle/VerificationReport 实际写出，保持外部命令编排和文件写入顺序不变。
- [x] `workbench_presenter_smoke` 已覆盖 UI adapter 的控件刷新和保存/报告触发动作。
- [x] 二阶段 final 输入组装已由 N62 抽到 workflow result builder；diff artifact 控件已由 N61 迁入独立 DiffPanel。

### 36.46 N60：testdiff 真实生成器 opt-in 契约与输出命名

- [x] 新增 `core/verify/TestdiffGenerationContract.*`，把真实生成器未来启用方式固化为 opt-in 契约。
- [x] 契约定义 `verification.testdiff_generation.enabled_generators` manifest 字段、默认禁用、Case 相对输出根 `artifacts/testdiff/generated` 和 sidecar 后缀 `.meta.json`。
- [x] 契约覆盖 `image_pixel_diff`、`property_structural_diff`、`performance_trend_diff` 三类生成器的 required inputs、generated role、输出文件模式和 sidecar 字段。
- [x] 契约明确隐私边界：生成 artifact 路径必须保持 Case 相对，sidecar 不记录 runner 输出绝对路径，私有 CAD/model 文件名在报告导出前必须脱敏。
- [x] `TestdiffGenerationPolicy::build()` 已在 `generation_policy.contract` 中携带该契约，但仍保持 `policy=boundary_only`、`generation_performed=false`、各生成器 `enabled=false`。
- [x] sample case 的 `artifacts/testgrid_result.json` 已补充 `generation_policy.contract` 示例，保持演示 schema 与当前代码一致。
- [x] 新增 `testdiff_generation_contract_smoke`，并扩展 `testdiff_generation_policy_smoke` 与 `testdiff_artifact_analysis_smoke` 覆盖 contract 集成路径。
- [ ] 当前仍不实现图片像素 diff、属性结构 diff 或性能趋势 diff 算法；后续真实生成器必须在 opt-in、容差/阈值、失败报告和隐私脱敏都有 smoke 覆盖后再启用。

### 36.47 N61：Diff Compare tab 独立 DiffPanel

- [x] 新增 `workbench/DiffPanel.*`，集中持有 Diff Compare tab 的摘要 label、artifact kind/status 过滤、artifact index 表、artifact analysis 表和当前 `testdiff_artifacts` 状态。
- [x] DiffPanel 复用 `DiffArtifactsPresenter` 刷新表格，保留 index 表按 diff/after/before 优先打开、analysis 表双击跳转到对应索引 artifact 的行为。
- [x] DiffPanel 通过 `generateTopologyCompareRequested` 和 `artifactOpenRequested(path, origin)` 信号把拓扑 compare 生成和 artifact 打开请求交回窗口层。
- [x] `WorkbenchWindow` 已不再持有差异表、过滤器或当前 artifact JSON；窗口层只负责读取 `artifacts/testgrid_result.json`、喂给 DiffPanel，并执行 Case workspace 内的安全打开/预览。
- [x] `workbench_presenter_smoke` 已覆盖 DiffPanel 的摘要刷新、表格刷新和首选 artifact 路径选择。
- [x] DiffPanel 已补充图片内嵌预览、路径复制和全文/文件名搜索；图片预览仍限制在当前 Case workspace 内的相对 artifact 路径。

### 36.48 N62：二阶段 final workflow result builder

- [x] 新增 `core/verify/TwoStageFinalResultBuilder.*`，集中组装二阶段 final writer/coordinator 需要的输入。
- [x] Builder 负责读取 before/after phase rows，生成 before/after comparison，解析 after command 或 summary 中的 testdiff，构造 failure_details、timing 和 `testdiff_artifacts`。
- [x] `WorkbenchWindow::persistTwoStageWorkflowResult()` 已改为调用 builder，再调用 final writer/coordinator；窗口层不再手写 comparison/testdiff/failure/timing/artifact 输入组装。
- [x] 新增 `two_stage_final_result_builder_smoke`，覆盖 writer input、回归对比、失败明细、耗时汇总、testdiff artifact 扫描和本机绝对路径不泄露。
- [x] EvidenceBundle/VerificationReport 实际写出已由 N63 抽到 `ReportRefreshCoordinator`。

### 36.49 N63：EvidenceBundle / VerificationReport report refresh coordinator

- [x] 新增 `workbench/ReportRefreshCoordinator.*`，集中处理 EvidenceBundle 与 VerificationReport 的实际写出、目标路径计算和错误汇总。
- [x] `WorkbenchWindow` 已改为提交 `ReportRefreshRequest`，由 coordinator 调用 `EvidenceBundleWriter` 与 `VerificationReportWriter`；窗口层只把错误输出到底部控制台。
- [x] 保持既有保存 Case、导出报告、patch/testgrid/testdiff/二阶段验证后的报告刷新顺序，不改变报告内容 writer。
- [x] 新增 `report_refresh_coordinator_smoke`，覆盖 evidence bundle、verification markdown、verification JSON 三类输出路径。
- [ ] `WorkbenchWindow` 仍保留 Case 保存、外部命令启动和 UI 刷新编排；后续继续收束这些职责。

### 36.50 N64：DiffPanel 图片预览、路径复制与搜索

- [x] `DiffArtifactsPresenter::Filter` 新增搜索文本，artifact index 与 analysis 行可按 kind、key、status、路径和分析摘要过滤。
- [x] `DiffPanel` 新增搜索框、`Copy path` 和 `Preview image` 入口；复制与预览都基于当前选中 artifact 的 Case 相对路径。
- [x] 图片预览请求通过 `artifactPreviewRequested` 交给 `WorkbenchWindow`，窗口层复用 Case workspace 安全路径校验后加载 `QPixmap` 并回填 panel。
- [x] `workbench_presenter_smoke` 已覆盖搜索过滤和预览状态。
- [ ] 预览区目前只做静态图片缩放展示；真实 testdiff 生成器启用后可继续补 before/after/diff 同屏比较和像素级标注。

### 36.51 N65：本地 build/run 脚本入口

- [x] 新增 `scripts/build.ps1`，统一封装 CMake configure、build 和 CTest，可通过 `-NoConfigure`、`-NoBuild`、`-NoTest`、`-TestRegex` 做最小验证。
- [x] 新增 `scripts/run.ps1`，统一定位并启动 `OCCTDebug.exe`，支持 `-BuildIfMissing` 在启动前补构建。
- [x] 脚本通过 `OCCTDEBUG_VSDEVCMD`、`vswhere` 或当前环境发现 Visual Studio developer environment，不写死个人机器路径。
- [x] README 已补充脚本运行方式，`DEVELOPMENT_CHECKLIST.md` 已更新基础工程验收状态。
- [ ] 脚本当前仍以 Debug preset 为默认入口；后续若增加 Release/RelWithDebInfo preset，应扩展参数和文档示例。

### 36.52 N66：CommandRunner 取消语义与 UI 入口

- [x] `CommandResult` 新增 `canceled` 字段，`CommandRunner::cancel()` 会在最终 result 中明确标记用户取消，不再只能靠退出码推断。
- [x] 新增 `command_runner_cancel_smoke`，启动长时间 PowerShell 命令后取消，验证 result 标记、输出未完成和 runner 状态恢复。
- [x] DRAW、环境采集、testgrid/testdiff/二阶段验证和 patch 命令已增加最小 `Cancel` 按钮，复用统一 `cancelRunner()` helper。
- [x] testgrid/testdiff/二阶段和 patch 编排收到 canceled result 后会停止后续阶段并恢复 phase/mode 状态。
- [ ] 当前取消入口仍是最小按钮；后续应补任务队列、按钮 enable/disable 状态和更细粒度取消日志 artifact。

### 36.53 N67：Shape 基础统计同步

- [x] `WorkbenchWindow::syncGeometryTopologyStats()` 会把 Viewer 的 `topologySummary()` 同步为几何检查表中的 `Topology stats` 行。
- [x] Demo shape 和导入模型加载成功后都会刷新该统计，并同步到 `CaseManifest::geometryChecks`。
- [x] 该能力复用现有 `OcctViewerWidget::topologySummary()`，当前不引入新的拓扑统计模型或第三方依赖。
- [ ] 当前统计仍是摘要文本行；后续可拆为专门的 topology stats 数据模型和表格列。

### 36.54 N68：输入文件 hash 记录与报告摘要

- [x] `CaseManifest` 新增 `InputFileRecord` 与 `input.files` JSON 契约，字段包含 `path`、`original_name`、`sha256`、`bytes` 和 `imported_at`。
- [x] 几何模型导入成功后会对复制到 Case `input/` 的目标文件计算 SHA-256，同一路径重复导入会更新记录而不是重复追加。
- [x] `MarkdownReportGenerator` 的 `repro_report.md` 新增“输入文件摘要”表，展示 Case 相对路径、原始文件名、大小、SHA-256 和导入时间。
- [x] `case_manifest_plan_smoke` 覆盖 `input.files` JSON 往返；`markdown_report_generator_smoke` 覆盖报告中的输入文件摘要与本机路径不泄露。
- [ ] 当前 hash 记录只接入几何模型导入入口；后续新建 Case 问题录入、普通附件导入和 case pack 导入也应复用同一契约。

### 36.55 N69：C++ 最小复现工程模板

- [x] 新增 `core/repro/CppReproTemplateWriter.*`，统一生成 `repro/cpp_minimal/CMakeLists.txt`、`main.cpp`、`README.md` 和 `repro_from_draw.tcl`。
- [x] 复现脚本 tab 新增 `C++ Repro` 入口，生成模板后会保存当前 DRAW 脚本、登记 Repro Evidence，并刷新 EvidenceBundle / VerificationReport。
- [x] 模板采用 `find_package(OpenCASCADE CONFIG QUIET)`，由使用者在独立构建时传入 `-DOpenCASCADE_DIR=<occt>/lib/cmake/opencascade`，不写入个人机器路径。
- [x] 新增 `cpp_repro_template_writer_smoke`，覆盖模板文件、当前 DRAW 脚本副本、Case id 注入和本机绝对路径不泄露。
- [ ] 当前模板只提供可编译的最小 OCCT C++ 工程骨架；后续仍需把真实 Case 的输入文件、几何构造和失败断言逐步映射为更贴近问题的 C++ 复现代码。

### 36.56 N70：UI 侧超时、取消和任务状态

- [x] `CommandRequest` 新增 `timeoutMs`，`CommandRunner` 可在命令超时时终止运行中的进程。
- [x] `CommandResult` 新增 `timedOut` / `timeoutMs`，与既有 `canceled` 一起形成最小任务结果状态。
- [x] DRAW、环境采集、testgrid/testdiff、二阶段验证和 patch 命令已设置统一超时常量，UI 日志输出 `passed/failed/canceled/timed_out`。
- [x] env/DRAW/repro pack/patch/testgrid/testdiff 相关 result artifact 已记录取消、超时和超时时间，便于后续报告与任务历史消费。
- [x] 新增 `command_runner_timeout_smoke`，覆盖长时间命令超时后 result 标记、输出截断和 runner 状态恢复。
- [ ] 当前仍是分散在各命令入口的最小任务状态记录；后续应补独立 TaskHistory/TaskPanel，统一显示任务队列、开始/结束时间、日志路径和取消入口。

### 36.57 N71：crash dump 文件归档

- [x] 新增 `core/case/CrashDumpArchive.*`，负责将 `.dmp/.mdmp/.dump` 复制到当前 Case `artifacts/crash/` 并计算 SHA-256。
- [x] 每个 dump 会生成同名 `.json` manifest，记录 Case 相对 artifact 路径、原始文件名、bytes、sha256 和归档时间，不写入源文件或 workspace 的本机绝对路径。
- [x] Evidence 面板新增 `Archive dump` 入口，归档成功后登记 `Crash Dump` Evidence，并刷新 EvidenceBundle / VerificationReport。
- [x] 新增 `crash_dump_archive_smoke`，覆盖归档文件、manifest schema、hash 一致性和本机绝对路径不泄露。
- [ ] 当前只做 dump 文件归档和证据登记，不解析 dump 内容；后续可在明确本地调试器/符号配置后接入栈摘要提取。

### 36.58 N72：复现状态判定与 Case 写回

- [x] `CaseManifest` 新增 `repro.status` 子对象，记录 `overall`、`draw`、`cpp`、`testgrid`、`updated_at` 和 `summary`。
- [x] 新增 `core/repro/ReproStatusEvaluator.*`，集中判定命令 passed/failed/canceled/timed_out、C++ scaffold generated 和 testgrid passed/failed/blocked 状态。
- [x] DRAW 运行结果落盘后会根据退出状态与 DRAW 日志分析写回 `repro.status.draw` 和 `overall`。
- [x] C++ 复现模板生成成功后会写回 `repro.status.cpp=generated`，不把模板生成误判为真实 C++ 复现通过。
- [x] testgrid 单阶段结果落盘后会写回 `repro.status.testgrid`，并把统一摘要同步为验证面板 `repro status` 指标。
- [x] 新增 `repro_status_evaluator_smoke`，并扩展 `case_manifest_plan_smoke` 覆盖 `repro.status` JSON 往返。
- [ ] 当前复现状态仍是聚合摘要；后续 TaskHistory/TaskPanel 应记录每次运行的时间线和日志路径，而不是只保留最后一次状态。

### 36.59 持续门禁

- [ ] 保持 `cmake --build out/build/debug --config Debug` 通过。
- [ ] 保持 `ctest --test-dir out/build/debug -R "draw_.*smoke" --output-on-failure` 通过。
- [ ] 保持 `ctest --test-dir out/build/debug --output-on-failure` 通过。
- [ ] 修改 `CaseManifest` / `WorkbenchMockData` / 核心公共 struct 后，优先执行一次 clean rebuild，避免本地 Ninja/MSVC 头依赖未触发导致旧对象 ABI 不一致。
- [x] 已检查 `cmake/occt_setup_install.cmake` 的 Release DLL 路径，当前指向 `lib/Release/bin`。

## 37. 结论

本 roadmap 的核心思想是：先把工具做成一个稳定、可信、可复现的 OCCT 问题工程工作台，再逐步增加诊断、补丁、验证和知识沉淀能力。

优先级排序应始终保持如下顺序：

```text
稳定运行 > 准确复现 > 清晰证据 > 可审查诊断 > 可验证补丁 > 知识沉淀 > 自动化增强
```

只要前期把 case 管理、环境快照、DRAW 复现、几何查看和报告导出打牢，后续无论增加自动补丁、AI 辅助分析、testgrid 验证还是企业知识库，都会有稳定的工程基础。
