# OCCT 内核专家工作台 Roadmap

> 项目代号：**OCCT Kernel Expert Workbench**  
> 当前阶段：旧代码已清理，新的 Qt Widgets 工作台骨架已启动，OCCT 与 Qt 本地依赖已配置  
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

本仓库不是全新空目录，当前根目录为：

```text
F:/data/github/OCCTDebug/
```

当前已经完成一次旧代码清理：旧模型查看器、旧 Shape 树、旧诊断规则、旧 IO、旧会话系统和旧 problem document importer 均已删除。需要旧实现时从 git 历史查找，不在当前源码树中保留无用代码。

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
    occt_3rdparty/freetype-2.13.3-x64/

  doc/
    OCCT_AutoFix_Workbench_Design.md
    OCCT_Kernel_Expert_Workbench_UI_Design.md
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
    workbench/
      WorkbenchWindow.h
      WorkbenchWindow.cpp

  tests/
    CMakeLists.txt
    shape_smoke.cpp

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
F:/data/github/OCCTDebug
```

当前 Debug 构建目录：

```text
F:/data/github/OCCTDebug/out/build/debug
```

当前应用输出：

```text
F:/data/github/OCCTDebug/out/build/debug/src/OCCTDebug.exe
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

当前 Qt 配置入口：

```text
src/QtWorkbenchDefaults.cmake
```

当前本机 Qt kit：

```text
D:/Programming/Qt/6.11.0/msvc2022_64
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

当前可用验证命令：

```powershell
cmd /c ""D:\Programming\VisualStudio\2026\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && cmake --build out\build\debug --config Debug && ctest --test-dir out\build\debug --output-on-failure"
```

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
  repo_root: "F:/data/github/OCCTDebug"
  build_root: "F:/data/github/OCCTDebug/out/build"
  case_root: "F:/data/github/OCCTDebug/cases"
  artifact_root: "F:/data/github/OCCTDebug/artifacts"

occt:
  bundled_root: "F:/data/github/OCCTDebug/depends/occt"
  source_root: ""
  build_root: ""
  install_root: "F:/data/github/OCCTDebug/depends/occt"
  casroot: "F:/data/github/OCCTDebug/depends/occt"
  drawexe: ""
  testgrid_root: ""

third_party:
  freetype_root: "F:/data/github/OCCTDebug/depends/occt_3rdparty/freetype-2.13.3-x64"

qt:
  root: "D:/Programming/Qt/6.11.0/msvc2022_64"

compiler:
  developer_command: "D:/Programming/VisualStudio/2026/Community/Common7/Tools/VsDevCmd.bat"
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

注意：上述 `drawexe`、`source_root`、`testgrid_root` 当前尚未在项目中接入，先留空；等 P2/P3 实现环境检测和 DRAW runner 时再由 UI 自动检测或用户填写。

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

状态：**基本完成，继续补配置系统**。

P0 已经完成一次关键调整：旧实现已经清理，当前源码树只保留新工作台骨架和最小基础设施。

已完成：

- [x] 顶层 `CMakeLists.txt`。
- [x] `cmake/` 中 OCCT / FreeType 查找脚本。
- [x] Qt Widgets 应用入口 `src/app/main.cpp`。
- [x] 新工作台主界面骨架 `src/workbench/WorkbenchWindow.*`。
- [x] 最小日志工具 `src/core/Logger.*`。
- [x] 最小 OCCT 链接 smoke test `tests/shape_smoke.cpp`。
- [x] 删除旧 GUI、旧诊断、旧 IO、旧会话、旧 Shape 树代码。
- [x] `README.md` 已说明当前重搭状态。

仍需完成：

- [ ] 统一 `CMakePresets.json`，避免团队共享配置依赖本地 `CMakeUserPresets.json`。
- [ ] 建立 `config/workbench.default.yaml`。
- [ ] 建立 `config/workbench.local.example.yaml`。
- [ ] 明确 `.gitignore` 中本地配置、case workspace、artifact、cache 的规则。
- [ ] 实现 `AppContext`，统一管理配置、路径、服务对象。
- [ ] 实现基础配置加载器。
- [ ] 实现日志文件输出，而不仅是内存/调试输出。
- [ ] 修正 `cmake/occt_setup_install.cmake` 中 Release DLL 目录疑似指向 Debug 的问题。

### 交付物

- 可启动的 Qt Widgets 工作台应用。
- 项目基础目录清晰、无旧代码干扰。
- 本地配置文件规划明确。
- OCCT 与 Qt 基础链接验证。

### 验收标准

- [x] 应用能在 Windows x64 上构建。
- [x] CTest smoke 能验证 OCCT 基础链接。
- [x] 启动入口是新的 `WorkbenchWindow`。
- [ ] 能读取默认配置和本地配置。
- [ ] 能在 UI 中显示 OCCT、Qt、MSVC、CMake 基础环境信息。
- [ ] 日志文件能正常写入。

## 9. P1：主界面与案例管理

### 当前状态

状态：**骨架已完成，真实数据未接入**。

当前 `WorkbenchWindow` 已经按 UI 设计图完成静态骨架：顶部状态栏、流程工具栏、左侧案例/流程/关键输入、中间源码/几何/证据/差异/环境 tab、右侧诊断/补丁/验证/相似案例、底部 DRAW/CMake/testgrid 控制台。

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

待完成：

- [ ] 拆分 `WorkbenchWindow.cpp`，避免单文件继续膨胀。
- [ ] 引入 `CaseManifest` / `WorkflowState` 数据模型。
- [ ] 将静态样例 case 改为真实 case 数据。
- [ ] 实现 case 创建、打开、保存。
- [ ] 实现左侧 case 列表筛选和状态刷新。
- [ ] 实现布局保存与恢复。
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
- [ ] 底部控制台能追加真实 runner 日志。

## 10. P2：环境采集与配置管理

### 目标

实现对 Windows、OCCT、Qt、CMake、MSVC、DRAWEXE、testgrid 等环境的自动检测和快照保存。

### 主要任务

- [ ] 实现环境配置对话框。
- [ ] 检测 OCCT 源码目录。
- [ ] 检测 OCCT build/install 目录。
- [ ] 检测 `DRAWEXE.exe`。
- [ ] 检测 `CASROOT`。
- [ ] 检测 CMake。
- [ ] 检测 MSVC / VS2022。
- [ ] 检测 Qt 运行环境。
- [ ] 检测 PATH 中依赖 DLL。
- [ ] 检测 Tcl/Tk、FreeType、TBB 等常见依赖。
- [ ] 保存 `env_snapshot.json`。
- [ ] 在 UI 中展示环境检查结果。

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

- [ ] 能检测并显示 OCCT 与 Qt 当前配置。
- [ ] 能检测 DRAWEXE 是否可运行。
- [ ] 能检测 CMake / MSVC 是否可用。
- [ ] 能导出完整环境快照。
- [ ] 配置错误时能给出明确提示。

---

## 11. P3：复现生成与执行引擎

### 目标

实现最重要的工程闭环基础能力：从 case 输入生成标准复现目录，并执行 DRAW / C++ 复现。

### 主要任务

- [ ] 实现新建问题对话框。
- [ ] 实现 case workspace 初始化。
- [ ] 实现输入文件导入与 hash 记录。
- [ ] 实现 DRAW 脚本编辑器。
- [ ] 实现 C++ 最小复现工程模板。
- [ ] 实现 DRAWEXE runner。
- [ ] 实现 PowerShell / CMake runner。
- [ ] 实现运行日志捕获。
- [ ] 实现退出码、异常、超时处理。
- [ ] 实现 crash dump 文件归档。
- [ ] 实现复现状态判定。
- [ ] 实现复现报告生成。

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
- [ ] 能运行 DRAW 脚本并捕获输出。
- [ ] 能显示复现成功 / 失败状态。
- [ ] 能生成 `repro_report.md`。

---

## 12. P4：几何查看与 Shape 检查

### 目标

实现 OCCT 几何查看和基础 Shape 检查能力，使开发者可以直接在工作台中观察输入模型、异常边/面、中间结果和修复结果。

### 主要任务

- [ ] 集成 OCCT Viewer 到 Qt 界面。
- [ ] 支持加载 BREP / STEP / IGES。
- [ ] 支持基础显示：shaded、wireframe、透明、边线。
- [ ] 支持选择 face / edge / vertex。
- [ ] 支持显示 subshape ID。
- [ ] 支持高亮异常 edge / face。
- [ ] 实现 Shape 统计：Vertices、Edges、Wires、Faces、Shells、Solids。
- [ ] 实现基础 checkshape 结果展示。
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

- [ ] 能加载并显示 BREP 模型。
- [ ] 能选择并高亮边/面。
- [ ] 能显示 Shape 拓扑统计。
- [ ] 能将异常对象与日志中的 ID 关联。
- [ ] 能保存视图截图。

---

## 13. P5：源码定位与证据链

### 目标

将运行日志、调用栈、源码文件、几何对象和复现脚本串联成可审查的证据链。

### 主要任务

- [ ] 实现源码文件浏览器。
- [ ] 实现代码编辑器基础能力。
- [ ] 支持行号、高亮、搜索。
- [ ] 支持从调用栈跳转源码文件。
- [ ] 支持从日志错误跳转证据项。
- [ ] 实现调用栈解析。
- [ ] 实现运行日志结构化解析。
- [ ] 实现 Evidence 数据模型。
- [ ] 实现证据链面板。
- [ ] 支持证据项与几何对象关联。
- [ ] 支持证据项与源码文件关联。
- [ ] 生成 `diagnosis_evidence.json`。

### 证据类型

- 崩溃调用栈。
- DRAW 运行日志。
- C++ 复现日志。
- ASan 报告。
- Shape check 结果。
- Shape 统计。
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

- [ ] 日志中的异常能进入证据链。
- [ ] 调用栈能显示并跳转源码。
- [ ] 证据项能关联 case、run、source、shape。
- [ ] 能生成结构化证据文件。

---

## 14. P6：诊断结论与相似案例

### 目标

在证据链基础上生成明确、可审查的诊断结论，并支持检索相似案例、源码位置和历史问题。

### 主要任务

- [ ] 实现诊断结论卡片。
- [ ] 实现候选根因数据结构。
- [ ] 实现置信度展示。
- [ ] 实现相关源码文件列表。
- [ ] 实现相似案例检索。
- [ ] 实现本地知识库索引。
- [ ] 支持对 `doc/`、历史 case、源码说明做本地搜索。
- [ ] 支持关键词搜索 OCCT 源码。
- [ ] 支持按 toolkit / package / class / function 组织检索结果。
- [ ] 生成 `diagnosis_report.md`。

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

### 主要任务

- [ ] 实现 PatchCandidate 数据模型。
- [ ] 实现补丁方案卡片。
- [ ] 实现 diff viewer。
- [ ] 实现补丁文件列表。
- [ ] 实现风险等级标记。
- [ ] 实现影响模块标记。
- [ ] 支持人工编辑 patch 说明。
- [ ] 支持应用 patch 到工作区。
- [ ] 支持撤销 patch。
- [ ] 支持生成回归测试草案。
- [ ] 支持生成 patch review 报告。

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

- [ ] 能显示补丁 diff。
- [ ] 能将 patch 应用到指定 OCCT worktree。
- [ ] 能撤销 patch。
- [ ] 能记录人工审查意见。
- [ ] 能导出 `.patch` 文件。

---

## 16. P8：回归验证与测试报告

### 目标

实现从原始问题复验、相关测试运行、testgrid 结果解析到验证报告生成的完整验证能力。

### 主要任务

- [ ] 实现验证计划对话框。
- [ ] 支持运行原始 repro。
- [ ] 支持运行新增测试。
- [ ] 支持运行指定 testgrid group / grid / case。
- [ ] 支持运行 ctest。
- [ ] 支持解析 testgrid 输出。
- [ ] 支持解析测试通过率、失败列表、耗时。
- [ ] 支持 before / after 结果对比。
- [ ] 支持性能变化记录。
- [ ] 支持生成 `verification_report.md`。
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
- [ ] 能解析并展示通过率。
- [ ] 能列出失败用例。
- [ ] 能生成 `verification_report.md`。

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
- [ ] Shape 基础统计。
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

当前状态：**进行中，核心骨架已达成**。

- [x] Qt Widgets 应用启动。
- [x] 新工作台第一屏启动。
- [x] OCCT 基础链接 smoke test 通过。
- [x] 旧代码清理完成。
- [ ] 配置文件可加载。
- [ ] 环境信息可在 UI 展示。
- [ ] OCCT Viewer 最小嵌入 demo。
- [ ] DRAWEXE 可由应用调用。

### R1：工作台基础版

- [ ] 主界面完整并拆分为可维护模块。
- [ ] Case 管理可用。
- [ ] 环境采集可用。
- [ ] DRAW 复现可运行。
- [ ] 日志可展示并落盘。

### R2：复现与几何版

- [ ] 模型导入。
- [ ] 几何显示。
- [ ] Shape 统计。
- [ ] 复现报告生成。

### R3：证据与诊断版

- [ ] 调用栈解析。
- [ ] 证据链管理。
- [ ] 源码跳转。
- [ ] 诊断报告生成。

### R4：补丁与验证版

- [ ] diff 查看。
- [ ] patch 应用 / 撤销。
- [ ] 验证计划。
- [ ] testgrid 结果解析。
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

## 33. 近期开发顺序建议

基于当前仓库状态，近期不应再做旧功能迁移，应该从新架构最小闭环开始。

建议顺序：

```text
1. 统一本地配置和 CMake preset
2. 建立 AppContext + ConfigService
3. 定义 CaseManifest / WorkflowState / WorkspaceLayout
4. 实现 case 目录创建和样例 case 加载
5. 将 WorkbenchWindow 静态数据替换为 case 数据绑定
6. 实现日志落盘和底部控制台追加
7. 实现环境采集与环境信息 tab
8. 实现 DRAWEXE 路径检测和 DrawRunner
9. 实现 DRAW 脚本编辑与运行
10. 接入 OCCT Viewer 最小 demo
11. 实现 BREP 加载和 Shape 基础统计
12. 生成第一份 repro_report.md
13. 实现 EvidenceBundle 第一版
14. 实现诊断报告导出
```

其中前 6 项完成后，即可从“静态 UI 骨架”进入“真实 case 工作台”；前 12 项完成后形成第一个可演示 MVP。

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

当前环境已经具备 OCCT 和 Qt，并且新工作台骨架已经存在。下一步立即任务应从配置和真实数据模型开始，而不是继续扩展静态 UI。

- [ ] 清理/统一 `CMakePresets.json`，确保团队共享 preset 不依赖本机 `CMakeUserPresets.json`。
- [ ] 明确 `src/QtWorkbenchDefaults.cmake` 为本地 Qt kit 配置入口，并保持 gitignore。
- [ ] 增加 `config/workbench.default.yaml`。
- [ ] 增加 `config/workbench.local.example.yaml`。
- [ ] 更新 `.gitignore`，覆盖 `config/workbench.local.yaml`、`cases/`、`artifacts/`、`knowledge/cache/` 等本地产物。
- [ ] 实现 `AppContext`。
- [ ] 实现 `ConfigService`，读取默认配置、本地配置和 CMake 推导路径。
- [ ] 定义 `CaseManifest`、`WorkflowState`、`WorkspaceLayout`。
- [ ] 创建第一个 sample case 目录结构。
- [ ] 将 `WorkbenchWindow` 中静态 case 数据替换为 sample case 数据。
- [ ] 为底部控制台实现 append log API。
- [ ] 在环境信息 tab 显示当前 Qt、OCCT、CMake、MSVC 路径和版本。
- [ ] 检查并修正 `cmake/occt_setup_install.cmake` 的 Release DLL 路径。
- [ ] 保持 `cmake --build out/build/debug --config Debug` 和 `ctest --test-dir out/build/debug --output-on-failure` 通过。

## 37. 结论

本 roadmap 的核心思想是：先把工具做成一个稳定、可信、可复现的 OCCT 问题工程工作台，再逐步增加诊断、补丁、验证和知识沉淀能力。

优先级排序应始终保持如下顺序：

```text
稳定运行 > 准确复现 > 清晰证据 > 可审查诊断 > 可验证补丁 > 知识沉淀 > 自动化增强
```

只要前期把 case 管理、环境快照、DRAW 复现、几何查看和报告导出打牢，后续无论增加自动补丁、AI 辅助分析、testgrid 验证还是企业知识库，都会有稳定的工程基础。
