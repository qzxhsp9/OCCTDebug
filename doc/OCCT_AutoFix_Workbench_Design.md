# OCCT 自动问题解决工具设计方案

> 面向 Windows 平台，围绕 Open CASCADE Technology（OCCT）的问题采集、复现、诊断、源码修复、回归验证和知识沉淀，构建一套可交互、可追溯、可验证的自动化工作台。

---

## 1. 背景与目标

几何内核已经发展多年，但真正成熟、可用、开源且工业级的选择并不多。国内许多企业基于 OCCT 进行二次研发，但在使用和开发过程中会遇到大量问题，其中一部分是 API 使用问题，更多则是 OCCT 自身在复杂几何、布尔、倒角、偏置、数据交换、性能、可视化等方面的鲁棒性问题。

这些问题往往有以下特点：

- 数据复杂，复现困难。
- 问题现象与根因距离很远。
- 调试成本高，需要熟悉 OCCT 源码、DRAW、测试系统和几何算法。
- 补丁风险大，一个 case 修好可能导致其他 case 退化。
- 企业内部问题和补丁难以沉淀为可复用知识。

因此，可以开发一套 **OCCT AutoFix Workbench**，帮助企业开发人员完成从问题描述、数据准备、复现、诊断、源码修改、回归验证到知识归档的完整闭环。

---

## 2. 产品定位

该工具不应只是一个“让大模型直接写 OCCT 补丁”的聊天机器人，而应是一个：

> **Windows 本地 OCCT 问题修复工厂。**

它的核心能力是把 OCCT 的源码、DRAW Test Harness、Visual Studio 调试、CMake/MSBuild 构建、GitHub Issues/PR、官方测试系统、企业内部案例和 LLM 推理能力接入一个统一闭环。

最终输出四类制品：

| 输出物 | 说明 |
|---|---|
| Repro Pack | 最小复现包，包括数据文件、DRAW 脚本、C++ 最小复现工程、环境信息、执行命令。 |
| Diagnosis Report | 问题分析报告，包括触发路径、相关源码、调用栈、几何状态、历史相似问题和可能根因。 |
| Patch Pack | 源码补丁、修改说明、新增/修改的 OCCT 测试用例、兼容性说明。 |
| Verification Report | 原问题是否解决、相关 testgrid 是否通过、性能/内存/几何结果是否退化、风险项。 |

核心原则：

> **一切以“可复现 + 可验证”为准，LLM 只做规划、搜索、推理和生成候选补丁，不能绕过构建与测试。**

---

## 3. 为什么以 DRAW/Test Harness 为核心

OCCT 官方提供的 DRAW Test Harness 是基于 Tcl 的命令解释器和图形测试系统，可用于测试、演示、创建、显示、修改曲线、曲面和拓扑形状，也支持通过 C++ 扩展新命令。

OCCT 官方自动测试系统本身也是围绕 DRAW Test Harness 组织的。标准测试位于 OCCT 源码根目录的 `tests` 子目录，按如下三层组织：

```text
test group
  └── test grid
        └── test case
```

因此，工具的复现层应优先生成 **DRAW Tcl 脚本**，其次才是 C++ 最小复现工程。

这样做的优势包括：

- DRAW 脚本更容易转化为 OCCT 官方风格的回归测试。
- 布尔、倒角、偏置、Shape Healing、STEP/IGES/XCAF 等问题都可以用 DRAW 命令快速复现。
- `testgrid` 能运行局部或全量测试，并输出结果报告。
- `testdiff` 可比较两次测试的 CPU 时间、内存使用和图像结果，适合验证性能和可视化退化。

工具的第一个大目标不是自动写代码，而是：

> **把用户描述转化为稳定、最小、可提交到 OCCT 测试体系中的复现用例。**

---

## 4. 总体架构

```text
用户/企业开发者
   │
   ▼
问题录入与交互澄清层
   │  自然语言描述、CAD 文件、代码片段、期望/实际结果、OCCT 版本、工具链
   ▼
问题标准化与分类层
   │  分类：崩溃、错误几何、性能、导入导出、构建链接、可视化、API 使用疑问
   ▼
复现生成层
   │  DRAW 脚本、C++ 最小复现、数据裁剪、环境快照
   ▼
Windows 执行实验室
   │  Git worktree、CMake、MSBuild/Ninja、DRAWEXE、testgrid、调试器、ASan
   ▼
源码/知识检索层
   │  OCCT 源码、官方文档、GitHub Issues/PR、tests/bugs、企业内部案例
   ▼
诊断 Agent
   │  调用栈、日志、源码路径、历史相似 bug、几何中间状态、根因假设
   ▼
补丁 Agent
   │  生成候选补丁、生成回归测试、解释风险、限制修改范围
   ▼
验证 Agent
   │  原始问题复验、相关测试组、全量/增量 testgrid、性能/内存/图像 diff
   ▼
报告与知识沉淀层
   │  Markdown/HTML 报告、patch、复现包、测试结果、案例入库
   ▼
人工审批/合并/提交 upstream
```

---

## 5. Windows 平台技术选型

Windows-only 是一个优势，可以把工具链收敛到稳定组合。

| 层级 | 推荐方案 |
|---|---|
| 桌面 UI | C# WPF / WinUI 3，或 Qt for Windows |
| 后端编排 | Python + FastAPI，或 C# Worker Service |
| 构建系统 | CMake Presets + Visual Studio Build Tools + Ninja 或 MSBuild |
| 源码索引 | clangd/libclang + ripgrep + Universal Ctags + Git blame/log |
| 调试执行 | cdb/WinDbg、Visual Studio Debugger、ProcDump、Windows Error Reporting dump |
| 内存错误检测 | MSVC AddressSanitizer，使用 `/fsanitize=address` |
| OCCT 执行 | DRAWEXE、testgrid、testdiff、自定义 DRAW command 插件 |
| 数据库存储 | SQLite / PostgreSQL + 文件制品仓库 |
| 检索 | 关键词 BM25 + 向量检索 + 源码符号图谱 |
| 任务隔离 | 每个 case 一个 Git worktree、独立 build/install/results 目录 |
| 报告 | Markdown + HTML + JSON manifest |

---

## 6. 核心模块设计

### 6.1 问题录入与数据准备模块

用户不能只输入“布尔失败”“倒角失败”这种模糊描述。工具应强制用户提供结构化问题信息。

示例问题描述格式：

```yaml
case_id: OCC-LOCAL-2026-0001
title: "Chamfer fails on bspline corner"
occt:
  version: "8.0.0"
  commit: "..."
  build_type: "RelWithDebInfo"
  compiler: "MSVC 2022 x64"
problem:
  category: "modeling/chamfer"
  symptom: "operation returns empty shape"
  expected: "chamfer should be generated on selected edges"
  actual: "BRepFilletAPI_MakeChamfer fails"
  deterministic: true
inputs:
  files:
    - model.step
    - original.cpp
  parameters:
    distance: 2.0
    tolerance: 1.0e-7
repro:
  user_steps: |
    load STEP, select edge 15, call chamfer
privacy:
  confidential: true
  allow_external_search: false
```

自动采集信息包括：

- Windows 版本、CPU/GPU、内存。
- Visual Studio/MSVC 版本。
- CMake 版本。
- OCCT commit、分支、补丁状态。
- 第三方库路径。
- `CASROOT`、`PATH`、Tcl/Tk、FreeType、TBB 等环境信息。
- 运行命令和环境变量。
- 输入文件 hash。
- 数据是否允许公开。

很多企业 OCCT 问题并不是算法错误，而是环境、版本、二进制混用或第三方库不一致导致的，因此环境快照必须强制采集。

---

### 6.2 问题分类器

分类器初期不需要复杂模型，可以使用规则 + 检索 + LLM 的组合。

建议问题分类如下：

| 大类 | 典型问题 | 首选复现方式 |
|---|---|---|
| 构建/链接/部署 | CMake、DLL、CRT、Tcl/Tk、FreeType、vcpkg | PowerShell repro |
| 崩溃/异常 | Access Violation、Standard_Failure、SIGSEGV | C++ repro + dump + ASan |
| 错误几何 | 空 shape、无效 shape、自交、拓扑缺失 | DRAW script + checkshape |
| 布尔/倒角/偏置 | hang、结果错误、异常 | DRAW script + CSF_DEBUG_BOP |
| STEP/IGES/XCAF | 颜色、层、装配、PMI、GD&T 丢失 | import/export roundtrip |
| 网格 | BRepMesh 错误、法向异常、性能慢 | mesh group tests |
| 可视化 | AIS、HLR、OpenGL、选择高亮 | screenshot/image diff |
| 性能 | HLR 慢、布尔慢、内存暴涨 | profiler + testdiff |
| API 使用 | Handle、Location、Tolerance、生命周期误解 | 文档 + sample + 最小代码 |

---

### 6.3 复现生成模块

这是最关键模块。没有复现，就不应该进入自动改源码阶段。

复现分三层。

#### 第一层：原始复现

保留用户原始工程、数据和命令，不做修改。

#### 第二层：标准复现

将原始问题转成统一 case 结构：

```text
case/
  input/
    model.step
    original.cpp
  repro/
    repro.tcl
    repro.cpp
    CMakeLists.txt
    run_repro.ps1
  env/
    env.json
    cmake-cache.txt
  logs/
```

#### 第三层：最小复现

自动缩小问题数据。

对于 CAD 文件，可以做：

- STEP/XCAF：删除 assembly 中无关 label，只保留触发对象。
- BRep：按 subshape 做 delta debugging，删除 face/edge/solid 后检查 failure 是否仍存在。
- 布尔/倒角/偏置：自动定位参与操作的 face/edge，保存最小 `.brep`。
- 性能问题：保留能触发性能异常的最小子装配。
- 可视化问题：保存 camera、display mode、GPU 信息和截图。

对于布尔类问题，应特别接入 OCCT 的 `CSF_DEBUG_BOP` 机制。开启该环境变量后，布尔操作在检测到无效输入或无效结果时，可以保存参数 shapes 和可复现问题的 DRAW 脚本。

---

### 6.4 Windows 执行实验室

每个问题都应在独立 workspace 中执行：

```text
workspaces/
  OCC-LOCAL-2026-0001/
    src/                  # git worktree
    build-debug/
    build-relwithdebinfo/
    build-asan/
    install/
    case/
    results-baseline/
    results-patched/
```

建议构建矩阵：

| 构建类型 | 用途 |
|---|---|
| Debug | 断点调试、对象观察、详细日志。 |
| RelWithDebInfo | 接近真实行为，同时保留符号。 |
| Release | 性能验证。 |
| ASan | 内存越界、use-after-free、double free 等问题检测。 |
| 企业自定义配置 | 适配企业内部 fork 和第三方依赖组合。 |

执行器应支持：

```powershell
.\configure.ps1 -Preset vs2022-relwithdebinfo
.\build.ps1 -Target TKBool
.\run-draw.ps1 repro.tcl
.\run-cpp-repro.ps1
.\run-testgrid.ps1 -Group bugs -Grid modalg
.\run-testdiff.ps1 baseline patched
```

为了提高效率，不应每次全量编译。应支持：

- 按 toolkit 增量构建。
- 失败后只重跑相关 test group。
- 夜间或人工触发全量 testgrid。
- 编译缓存、build cache、PDB 缓存。

---

### 6.5 知识库与检索模块

知识库分为公开知识与企业私有知识。

#### 公开知识

- OCCT GitHub 源码。
- GitHub Issues / PR / Discussions。
- 官方文档和用户指南。
- 官方升级指南。
- 官方 `tests` 目录，尤其是 `tests/bugs`。
- 官方 release notes。
- 社区论坛、FreeCAD、Analysis Situs 等相关问题。

#### 企业私有知识

- 企业历史 bug。
- 内部补丁。
- 内部 CAD 数据的匿名化特征。
- 内部开发规范。
- 以前的问题分析报告。
- 失败尝试和最终 patch。

#### 检索方式

不能只做向量检索，必须做混合检索：

```text
关键词检索：BRepAlgoAPI_Section, ChFi3d, ShapeFix, XCAFDoc, STEPCAFControl
符号检索：class/function/file/toolkit dependency
语义检索：用户描述、错误现象、历史 issue
Git 检索：blame、log、diff、相邻修改
测试检索：相似 DRAW 命令、tests/bugs 中相同功能
```

建议为每个知识片段打标签：

```yaml
entity:
  toolkit: TKFillet
  package: ChFi3d
  class: BRepFilletAPI_MakeChamfer
  function: Perform
problem_type: wrong_result
source_type: github_issue
confidence: 0.82
version_range: "7.6.0-8.0.0"
```

---

### 6.6 诊断 Agent

诊断 Agent 的任务不是马上修代码，而是生成“可审查的根因假设”。

每个假设必须包含证据链：

```yaml
hypothesis_id: H1
summary: "倒角失败可能由 BSpline corner 处 edge-face pcurve tolerance 不一致触发"
evidence:
  - "repro.tcl 在 8.0.0 RelWithDebInfo 稳定失败"
  - "checkshape 输入 shape OK，结果 shape empty"
  - "调用栈停在 ChFi3d_Builder::PerformIntersectionAtEnd"
  - "tests/bugs/modalg 中有相似 corner/chamfer 用例"
candidate_files:
  - src/ChFi3d/ChFi3d_Builder.cxx
  - src/BRepFilletAPI/BRepFilletAPI_MakeChamfer.cxx
risk:
  - "可能影响 fillet/chamfer 通用分支"
next_action:
  - "加日志观察 intersection result"
  - "尝试 tolerance fallback patch"
```

诊断 Agent 应使用以下证据：

- 崩溃 dump 和调用栈。
- ASan 报告。
- `checkshape` / `checkfaults`。
- DRAW 中间 shape dump。
- Git blame 和历史补丁。
- 相似 tests/bugs。
- 相似 GitHub issue。
- 性能 profile。
- 输入输出拓扑统计：face/edge/vertex 数量、tolerance 分布、bounding box、closed/manifold 状态。

OCCT 调试能力中还包含从 Visual Studio debugger 调用 DRAW 相关函数、保存/dump shape、将 shape 放入 DRAW 变量、JSON dump、natvis 等能力。工具可以将这些能力封装为按钮或自动脚本，例如：

> 在断点处保存当前 `TopoDS_Shape` 到 `.brep` 并自动打开 DRAW 查看。

---

### 6.7 补丁 Agent

补丁 Agent 必须被严格约束，不能随意大改内核。

建议规则：

1. **优先小补丁**：只修改与复现强相关的局部分支。
2. **必须附带回归测试**：没有 test case 的 patch 不进入候选合并。
3. **不能吞异常**：不能简单使用 `catch (...) return;`。
4. **不能粗暴放大 tolerance 掩盖问题**，除非报告说明几何依据和影响范围。
5. **不能修改公开 API**，除非用户明确选择“破坏兼容”模式。
6. **必须格式化代码**，例如使用 OCCT 当前代码风格要求的 clang-format。
7. **必须说明风险面**：影响哪些 toolkit、哪些算法、哪些测试组。

补丁生成可以分三档：

| 档位 | 自动化程度 | 适用场景 |
|---|---|---|
| Safe Patch | 高 | 空指针、边界检查、异常路径、资源释放、明显回归。 |
| Assisted Patch | 中 | tolerance、拓扑状态修正、导入导出字段遗漏。 |
| Research Patch | 低 | 布尔核心、曲面相交、倒角/圆角复杂算法、性能算法重构。 |

复杂算法不是不能做，而是要放进更强的验证流程：

- 更多中间状态记录。
- 更多同类测试。
- 更多人工审批点。
- 更严格的性能和鲁棒性比较。

---

### 6.8 验证 Agent

验证不能只看原问题是否通过。

建议验证分级：

```text
V0: 编译通过
V1: 原始 repro 失败 -> patch 后成功
V2: 新增 DRAW/C++ 回归测试通过
V3: 相关 group/grid 通过，例如 bugs modalg、offset、heal、step、xde
V4: testdiff 无明显性能/内存/图像退化
V5: 企业内部历史用例集通过
V6: 全量 testgrid 或 CI 通过
```

验证配置示例：

```yaml
verification:
  must_pass:
    - repro/original
    - tests/bugs/modalg
  should_pass:
    - tests/chamfer/*
    - tests/offset/*
  performance:
    compare_with: baseline
    max_regression_percent: 5
  artifacts:
    - summary.html
    - junit.xml
    - diff.html
```

---

### 6.9 报告与审计模块

每个 case 最终都应生成一份可供企业内核开发人员审查的报告。

报告结构建议：

```text
1. 问题摘要
2. 环境信息
3. 输入数据
4. 最小复现步骤
5. 原始失败证据
6. 源码定位过程
7. 根因假设与证据
8. 修改内容
9. 新增测试
10. 验证结果
11. 风险与未覆盖场景
12. 是否建议提交 upstream
```

报告要记录每一次 Agent 的关键动作：

- 查了哪些源码。
- 跑了哪些命令。
- 哪些假设被否定。
- patch 为什么这样改。
- 哪些测试失败但被判定为无关。
- 哪些数据不能公开。

这对企业非常重要，因为 OCCT 内核问题往往会反复出现。今天解决一个 case，明天就可以沉淀成可复用知识。

---

## 7. Agent 状态机流程

工具应设计成状态机，而不是自由聊天。

```text
NewIssue
  -> NeedMoreInfo?
  -> PrepareEnvironment
  -> BuildBaseline
  -> Reproduce
  -> MinimizeRepro
  -> RetrieveKnowledge
  -> Diagnose
  -> GeneratePatch
  -> BuildPatched
  -> Verify
  -> HumanReview
  -> ArchiveCase
```

伪代码：

```python
def solve_case(case):
    env = collect_environment(case)
    baseline = build_occt(case.repo, env, mode="RelWithDebInfo")

    repro = make_reproducer(case)
    result0 = run_reproducer(baseline, repro)

    if not result0.reproduced:
        return ask_or_report_missing_repro(case, result0)

    minimized = minimize_reproducer(repro, result0)
    evidence = collect_evidence(baseline, minimized)

    knowledge = retrieve_related_knowledge(
        description=case.description,
        symbols=evidence.symbols,
        stack=evidence.stack,
        tests=evidence.related_tests,
    )

    hypotheses = diagnose(evidence, knowledge)

    for h in hypotheses.rank():
        patch = generate_patch(h)
        build = build_occt(case.repo.apply(patch), env, mode="RelWithDebInfo")
        if not build.ok:
            continue

        verification = verify_patch(build, minimized, h.related_tests)
        if verification.acceptable:
            return create_patch_pack(case, h, patch, verification)

    return create_diagnosis_report(case, hypotheses)
```

---

## 8. 交互提问策略

工具允许交互提问，但不能每一步都问用户。建议只在三种情况下提问。

### 8.1 复现不完整

例如：

- 缺少 CAD 文件。
- 缺少操作参数。
- 无法确定选择哪条 edge。
- 无法知道用户期望结果。

### 8.2 需求有歧义

例如：

- 布尔结果为空，用户是希望返回空 shape、抛异常，还是自动修复输入？
- 导入 STEP 后颜色丢失，是要求完全还原颜色，还是允许通过附加映射恢复？

### 8.3 风险需要授权

例如：

- 是否允许修改公开 API？
- 是否允许上传 issue？
- 是否允许脱敏 CAD 数据？
- 是否接受性能换鲁棒性？

交互问题应非常具体。

示例：

```text
我已能稳定复现倒角失败，但无法判断用户期望的倒角边集合。
请选择：
A. 使用原代码中的全部 selected edges
B. 只使用模型中 ID=15 的 edge
C. 让我自动搜索能触发失败的最小 edge 集合
```

---

## 9. OCCT 常见问题专项处理器

### 9.1 布尔 / Section / 切割 / 相交

处理流程：

```text
启用 CSF_DEBUG_BOP
运行原始操作
保存 argument shapes 和 DRAW script
checkshape 输入和结果
分析 BOPAlgo/BRepAlgo 调用栈
提取相交曲线/边/face
尝试最小化参与 shape
生成 tests/bugs/modalg 用例
```

适用问题：

- section hang。
- fuse/cut/common 结果错误。
- 交线缺失。
- 自交。
- Boolean 后 shape invalid。

---

### 9.2 倒角 / 圆角 / 偏置

处理流程：

```text
识别 edge/face 集合
记录半径/距离/模式
检查 edge pcurve、same parameter、tolerance
保存失败前中间 spine/contour
定位 ChFi3d / BRepFillet / BRepOffset 相关源码
跑 fillet/chamfer/offset 相关 tests
```

适用问题：

- Chamfer fails for BSpline corner。
- Fillet cannot meet opposite edges。
- Offset 结果破面。
- 倒角产生非法拓扑。

---

### 9.3 STEP / IGES / XCAF 数据交换

处理流程：

```text
导入 -> XCAF tree dump
导出 -> 再导入 -> tree diff
比较 shape、label、name、color、layer、material、PMI/GD&T
定位 STEPCAFControl、XCAFDoc、RWStep、Interface_Static 参数
生成最小 STEP/XBF/BREP 数据
```

适用问题：

- STEP 读入实体缺失。
- 颜色/层/装配结构丢失。
- GD&T presentation 缺失。
- 单位或坐标系不对。

---

### 9.4 崩溃 / 内存错误

处理流程：

```text
Debug/RelWithDebInfo 复现
收集 dump + call stack
ASan build 复现
定位越界/use-after-free/double free
最小化输入数据
补丁后跑原 repro + 相关 tests
```

适用问题：

- Access Violation。
- Use-after-free。
- Double free。
- 越界访问。
- 未初始化内存导致的不稳定行为。

---

### 9.5 性能问题

处理流程：

```text
固定输入和机器配置
Release/RelWithDebInfo 对比
记录 wall time、CPU time、memory
采样 profiler
定位热点函数
patch 后用 testdiff 比较
```

适用问题：

- HLR 慢。
- 布尔运算慢。
- STEP 导入导出慢。
- 内存暴涨。
- 网格生成耗时异常。

---

## 10. 项目目录建议

```text
occt-autofix/
  apps/
    desktop/                 # Windows UI
    cli/                     # 命令行入口
  services/
    orchestrator/            # 状态机、任务调度
    artifact-store/          # 日志、报告、patch、数据管理
  agents/
    intake-agent/
    repro-agent/
    diagnose-agent/
    patch-agent/
    verify-agent/
    review-agent/
  occt/
    build-presets/
    draw-plugins/
    test-templates/
    powershell/
  knowledge/
    crawlers/
      occt-docs/
      github-issues/
      github-prs/
      forum/
    indexer/
    retrieval/
  reducers/
    brep-reducer/
    step-xcaf-reducer/
    cpp-reducer/
  runners/
    cmake-runner/
    draw-runner/
    testgrid-runner/
    debugger-runner/
    asan-runner/
  reports/
    templates/
  cases/
    OCC-LOCAL-*/             # 每个问题一个 case
```

---

## 11. MVP 路线

不要一开始就做“全自动修复所有 OCCT 问题”。建议分阶段建设。

### MVP-1：问题采集 + 环境快照 + 自动复现

目标：

- 用户上传问题描述、代码和 CAD 数据。
- 自动采集 Windows / VS / CMake / OCCT 环境。
- 自动生成 case 目录。
- 能运行 DRAWEXE 和 C++ repro。
- 能生成 Repro Pack 和失败报告。

这一版不需要自动补丁，但要把“复现”做扎实。

---

### MVP-2：知识库 + 源码定位 + 诊断报告

目标：

- 索引 OCCT 源码、官方文档、tests、GitHub Issues。
- 对崩溃问题自动解析调用栈。
- 对几何问题自动运行 `checkshape`、导出中间 shape。
- 输出候选源码文件和根因假设。
- 报告中列出相似 issue、test 和 source。

---

### MVP-3：自动生成回归测试

目标：

- 把复现转成 OCCT 风格 test case。
- 自动放到 `tests/bugs/<grid>/<case>`。
- 自动维护 `grids.list` / `cases.list`。
- 自动跑相关 `testgrid`。
- 输出可提交的测试补丁。

这一版非常关键，因为没有测试就没有可靠自动修复。

---

### MVP-4：受控自动补丁

先从高成功率问题做：

- 崩溃保护。
- 空指针 / 越界。
- 异常路径。
- 明确导入导出字段遗漏。
- 明确回归。
- 构建脚本 / 路径 / 环境问题。
- 简单 tolerance 分支修正。

暂缓大规模自动修改：

- 曲面相交核心算法。
- Boolean 核心重写。
- Fillet / Chamfer 全局策略调整。
- 大范围性能重构。

---

### MVP-5：企业知识闭环

目标：

- 每个 case 自动归档。
- 私有 patch 与 upstream patch 关联。
- 重复问题自动识别。
- 给企业 fork 做升级冲突和补丁迁移辅助。
- 对 OCCT 新版本自动跑企业回归集。

---

## 12. 评价指标

工具是否成功，不应只看“自动改了多少代码”，而应看以下指标：

| 指标 | 含义 |
|---|---|
| 复现成功率 | 用户问题能否被转成稳定 repro。 |
| 最小化成功率 | 大 CAD 文件能否裁剪成小数据。 |
| 诊断命中率 | 候选源码文件是否包含最终修改点。 |
| Patch 一次通过率 | 自动补丁首次通过验证的比例。 |
| 回归拦截率 | 自动发现补丁副作用的能力。 |
| 平均 case 沉淀质量 | 报告、测试、数据是否可复用。 |
| 企业升级收益 | 新 OCCT 版本上历史问题自动检测比例。 |

---

## 13. 必须注意的风险

### 13.1 数据保密

很多企业 CAD 数据不能外发。工具应默认本地运行，公开检索和 LLM 调用必须有开关。

对于私有数据，应优先做：

- hash。
- 拓扑统计。
- 匿名化。
- 子 shape 裁剪。
- 脱敏后再判断是否允许外部使用。

---

### 13.2 LLM 幻觉

所有补丁必须经过：

- 编译。
- 原始复现验证。
- 新增测试验证。
- 相关 testgrid 验证。
- testdiff 验证。

报告中必须区分：

- 事实证据。
- 模型推断。
- 人工确认。

---

### 13.3 几何 tolerance 风险

OCCT 问题很容易被“放大容差”掩盖。

任何 tolerance 改动都必须记录：

- 输入 tolerance 分布。
- 输出 tolerance 分布。
- 修改前后几何差异。
- 影响范围。
- 相关回归测试。

---

### 13.4 性能退化

几何内核补丁可能让一个 case 变好，却让多个 case 变慢。

因此必须集成：

- 性能基线。
- 内存基线。
- 局部 testdiff。
- 企业历史性能 case。

---

### 13.5 许可证与合规

OCCT 使用 LGPL 2.1 加 special exception，并提供商业许可选项。企业 fork 和自动生成补丁需要注意：

- 保留 license notice。
- 保留第三方依赖清单。
- 记录修改来源。
- 记录自动生成补丁和人工修改补丁的边界。
- 区分可 upstream 的补丁和企业私有补丁。

---

## 14. 推荐的第一个垂直样板

建议选择一个典型问题作为样板：

> 某 STEP 模型导入后做倒角失败，OCCT 8.0.0 + VS2022，企业产品中返回空 shape。

工具流程应完整覆盖：

1. 用户上传 STEP、原始 C++ 片段、期望截图。
2. 工具采集环境。
3. 工具生成 `repro.cpp` 和 `repro.tcl`。
4. DRAW 中稳定复现。
5. 自动最小化 STEP/BREP。
6. 自动定位到 BRepFillet / ChFi3d 相关源码。
7. 检索 GitHub Issues、tests/bugs、官方文档。
8. 生成诊断报告。
9. 生成 `tests/bugs/modalg/xxx` 回归测试。
10. 生成候选 patch。
11. 编译 patch。
12. 跑原 repro、相关 testgrid、testdiff。
13. 输出 patch pack 和 verification report。

这个样板一旦打通，布尔、偏置、STEP/XCAF、崩溃类问题都可以沿用同一套框架扩展。

---

## 15. 最终形态

成熟后的工具应像一个“内核专家工作台”：

- 左侧是问题列表和状态机。
- 中间是复现、日志、源码、调用栈、几何视图。
- 右侧是 Agent 诊断、相似案例、候选 patch、验证结果。
- 下方是 DRAW 控制台、PowerShell 命令、testgrid 结果。
- 每个按钮背后都是可追溯脚本，而不是黑盒操作。

最终价值在于：

> 企业开发人员不再从“用户一句话 + 一个大模型 + 一堆源码”开始盲查，而是从一个标准化的 **复现包、证据链、候选源码、历史案例、回归测试和补丁验证报告** 开始工作。

对于 OCCT 这种复杂几何内核，这比单纯让 AI 写代码可靠得多，也更容易逐步积累成企业自己的 OCCT 问题知识库。

---

## 16. 参考资料

- [OCCT 官方 GitHub 仓库](https://github.com/Open-Cascade-SAS/OCCT)
- [OCCT 官方 GitHub Issues](https://github.com/Open-Cascade-SAS/OCCT/issues)
- [OCCT 官方文档：DRAW Test Harness](https://dev.opencascade.org/doc/overview/html/occt_user_guides__test_harness.html)
- [OCCT 官方 Wiki：Tests](https://github.com/Open-Cascade-SAS/OCCT/wiki/tests)
- [OCCT 官方文档：Debugging](https://dev.opencascade.org/doc/overview/html/occt__debug.html)
- [OCCT 官方文档：Building OCCT](https://dev.opencascade.org/doc/overview/html/build_upgrade__building_occt.html)
- [OCCT 官方文档：Upgrade Guide](https://dev.opencascade.org/doc/overview/html/occt__upgrade.html)
- [OCCT 官方 Release 页面](https://dev.opencascade.org/release)
- [OCCT Licensing](https://dev.opencascade.org/resources/licensing)
- [Microsoft 文档：AddressSanitizer for MSVC](https://learn.microsoft.com/en-us/cpp/sanitizers/asan?view=msvc-170)

