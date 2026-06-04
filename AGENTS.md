# AGENTS.md — OCCTDebug 项目级 Codex 指令

## 1. 项目定位

OCCTDebug 是一个仅面向 Windows 平台的 OCCT 内核专家工作台项目，目标是帮助企业开发者围绕 OCCT 问题完成：

1. 问题描述与环境快照采集。
2. DRAW / C++ 最小复现生成。
3. OCCT 源码、官方文档、Issue、历史案例联合检索。
4. 调用栈、几何状态、日志、测试结果驱动的根因分析。
5. 候选补丁与回归测试生成。
6. 本地构建、testgrid / testdiff 验证。
7. 报告归档与知识库沉淀。

核心原则：任何分析、补丁或结论都必须可复现、可审查、可验证。不要只给“猜测式”修改。

## 2. 当前技术约束

- 平台：Windows x64 only。
- UI：Qt / Qt Widgets 或 Qt Quick，优先保持工程化、简洁、高效。
- 几何内核：Open CASCADE Technology，简称 OCCT。
- 典型工具链：Visual Studio 2022、CMake、PowerShell、DRAWEXE、testgrid、testdiff。
- 初期重点：工作台 UI 骨架、Case 管理、环境采集、复现生成、日志/证据展示、验证流程封装。
- 不要默认引入跨平台复杂性；如果必须兼容 Linux/macOS，先在方案中说明，但不要影响 Windows MVP。

## 3. 重要文档入口

优先阅读以下文档，再开始修改代码：

- `doc/roadmap.md`：最终开发路线图与里程碑。
- `doc/OCCT_AutoFix_Workbench_Design.md`：总体工具方案。
- `doc/OCCT_Kernel_Expert_Workbench_UI_Design.md`：详细 UI 设计。
- `doc/CODEX_CONTEXT.md`：面向 Codex 的项目上下文摘要。
- `doc/CODEX_TASKS.md`：推荐任务拆解与提示词模板。
- `doc/DEVELOPMENT_CHECKLIST.md`：开发验收清单。

如果文档名在仓库中略有差异，先搜索 `doc/` 目录下与 roadmap、UI、AutoFix、Workbench、OCCTDebug 相关的 Markdown 和图片。

## 4. 推荐仓库结构

目标结构如下；若当前结构不同，应尽量渐进式迁移，不要一次性大改目录：

```text
OCCTDebug/
  AGENTS.md
  README.md
  CMakeLists.txt
  .codex/
    config.toml
  doc/
    roadmap.md
    OCCT_AutoFix_Workbench_Design.md
    OCCT_Kernel_Expert_Workbench_UI_Design.md
    CODEX_CONTEXT.md
    CODEX_TASKS.md
    DEVELOPMENT_CHECKLIST.md
    images/
  app/
    OCCTDebugApp/
  src/
    core/
    ui/
    case/
    runner/
    occt/
    report/
    knowledge/
  tests/
  scripts/
    build.ps1
    run.ps1
    verify_env.ps1
```

## 5. 代码设计原则

- UI 与业务逻辑分离：Qt 界面只做展示和交互，Case 状态、任务编排、执行器、报告生成放在独立模块。
- 每个 Case 都是可归档对象，至少包含：描述、环境、输入文件、复现脚本、日志、证据、补丁、验证结果。
- 所有外部命令调用必须记录：命令、工作目录、环境变量摘要、stdout、stderr、退出码、耗时。
- Windows 路径处理要健壮，避免硬编码 `C:\OCCT`，优先通过配置项和环境变量。
- 对 OCCT 源码修补保持克制：优先生成补丁候选和回归测试，不要盲目大改核心算法。
- 对保密数据默认本地处理，不上传，不打印完整绝对路径和敏感文件名到公开报告。

## 6. UI 实现原则

最终界面是“内核专家工作台”，不是营销看板：

- 主布局：顶部工具栏 + 左侧 Case/流程 + 中央源码/几何/证据 + 右侧诊断/补丁/验证 + 底部控制台。
- 视觉风格：深色主题、少量蓝色强调、清晰分区、低噪声、高信息密度。
- 交互优先级：能一眼看到当前 Case 状态、复现结果、根因结论、候选补丁、验证指标。
- 每个自动化动作都应能展开查看命令和日志。
- 初期可使用 mock 数据，但模型类和 UI 控件命名要贴近真实业务。

## 7. 构建与验证约定

优先使用仓库中的脚本；如果缺失，可先补齐脚本再运行：

```powershell
./scripts/verify_env.ps1
./scripts/build.ps1 -Config Debug
./scripts/run.ps1
```

若项目采用 CMake + Qt：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

完成代码修改前，至少执行能够运行的最小构建或静态检查；如果因本地缺少 OCCT/Qt 路径无法构建，应在回复中明确说明缺失条件，并保留改动的可验证路径。

## 8. Definition of Done

一次 Codex 任务只有在满足以下条件时才算完成：

1. 修改范围与任务目标一致，没有无关大改。
2. 代码能编译，或明确说明为什么当前环境无法编译。
3. 新增 UI / 业务模块有清晰的数据模型或 mock 数据入口。
4. 关键行为有测试、脚本或手动验证步骤。
5. 变更说明包含：改了什么、为什么改、如何验证、风险点。
6. 不泄露用户私有 CAD 数据、公司路径、密钥、许可证文件。

## 9. 禁止事项

- 不要把 OCCT 问题简单归因于“使用问题”，必须先建立复现和证据。
- 不要用吞异常、扩大 tolerance、跳过失败测试来伪造修复。
- 不要在没有用户授权时删除或移动大批文件。
- 不要默认修改第三方库、OCCT 源码或系统环境变量。
- 不要将私有 CAD 数据、内部日志、授权文件加入 Git。
- 不要生成只适合演示、不适合工程维护的 UI 代码。

## 10. 开发节奏建议

优先按以下顺序推进：

1. 项目骨架与 CMake/Qt 启动。
2. 主窗口 UI 框架与主题。
3. Case 数据模型与本地文件结构。
4. 环境采集与命令执行器。
5. DRAW / C++ 复现脚本管理。
6. 日志、调用栈、几何信息、测试结果面板。
7. 报告生成。
8. 知识检索与 Agent 编排。
9. OCCT 修补与 testgrid/testdiff 自动验证。
