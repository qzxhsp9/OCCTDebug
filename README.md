# OCCT Kernel Expert Workbench

面向 Windows + Qt + OCCT 的内核问题工程化工作台。当前项目已放弃旧的模型查看器/诊断器实现，重新按以下文档搭建：

- `doc/OCCT_AutoFix_Workbench_Design.md`
- `doc/OCCT_Kernel_Expert_Workbench_UI_Design.md`
- `doc/occt内核专家工作台分析界面.png`

目标不是做普通几何查看器，而是逐步实现：

```text
问题录入 -> 环境采集 -> 自动复现 -> 数据最小化 -> 源码分析 -> 根因诊断 -> 补丁方案 -> 回归验证 -> 知识归档
```

## 当前状态

当前代码只保留重搭所需的最小骨架：

| 路径 | 说明 |
|---|---|
| `src/app/` | 应用入口，启动新的工作台主窗口 |
| `src/workbench/` | 新的 OCCT 内核专家工作台 UI 骨架 |
| `src/core/app/` | `AppContext`，统一暴露仓库、构建、Case 和依赖路径 |
| `src/core/config/` | `ConfigService`，读取默认配置和本地配置 |
| `src/core/Logger.*` | 最小日志工具 |
| `tests/` | 最小 OCCT 链接 smoke test |
| `cmake/` | OCCT / FreeType 查找脚本 |
| `depends/` | 本地 OCCT / FreeType 依赖 |
| `config/` | 工作台默认配置和本地配置模板 |
| `cases/` | 本地 Case workspace，运行时生成并被 gitignore |
| `doc/` | 新产品和 UI 设计文档 |

旧 GUI、旧 Shape 树、旧诊断规则、旧会话和旧导入导出代码已删除；需要时从 git 历史查找。

## 依赖

- CMake 3.20+
- 支持 C++17 的 MSVC / GCC / Clang
- Qt6 Core / Gui / Widgets
- OCCT，本项目按 `cmake/occt_setup_install.cmake` 从 `depends/occt` 解析
- FreeType，本项目按 `cmake/occt_3rdpart_setup_install.cmake` 从 `depends/occt_3rdparty` 解析

## 构建

推荐使用项目脚本：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build.ps1
powershell -ExecutionPolicy Bypass -File scripts/run.ps1
```

只运行指定 CTest：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -NoConfigure -NoBuild -TestRegex draw_smoke
```

启动前自动构建：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/run.ps1 -BuildIfMissing
```

示例：

```powershell
cmake --preset OCCTDebug-Debug
cmake --build out/build/debug --config Debug
ctest --test-dir out/build/debug --output-on-failure
```

如果直接使用 Visual Studio Developer Command Prompt：

```powershell
cmake -S . -B out/build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build/debug --config Debug
ctest --test-dir out/build/debug --output-on-failure
```

## 配置

共享默认配置位于 `config/workbench.default.yaml`。当前文件使用 JSON-compatible YAML 子集，便于用 Qt 原生 JSON parser 读取，后续如需完整 YAML 再单独引入解析策略。

本地配置模板位于 `config/workbench.local.example.yaml`。需要覆盖个人路径时复制为 `config/workbench.local.yaml`，该文件已被 `.gitignore` 忽略。

个人 Qt kit 路径仍优先放在 `src/QtWorkbenchDefaults.cmake` 或环境变量 `QTDIR` 中，不写入共享配置。

## Case Workspace

应用启动时会确保本地 `cases/OCC-LOCAL-2026-0001/` sample workspace 可用。该目录由 `sample_cases/OCC-LOCAL-2026-0001/` 初始化，包含：

```text
case.json
input/
repro/
env/
logs/
artifacts/
report/
verification/
```

`cases/` 是本地工作区，已被 `.gitignore` 忽略。

当前 UI 会优先从该 workspace 加载 sample Case。复现脚本页保存/运行 DRAW 时会写入：

```text
repro/repro.tcl
logs/draw.stdout.log
logs/draw.stderr.log
artifacts/draw_result.json
artifacts/draw_evidence.json
artifacts/draw_log_analysis.json
```

环境信息页的“采集环境”会调用 `scripts/verify_env.ps1`，并写入：

```text
env/env_snapshot.json
logs/env_capture.stdout.log
logs/env_capture.stderr.log
artifacts/env_capture_result.json
```

几何视图页可将 `.brep`、`.step`、`.stp`、`.iges`、`.igs` 导入当前 Case，并加载到 OCCT Viewer：

```text
input/<model>.brep
input/<model>.step
input/<model>.iges
```

验证结果面板可导出当前 Case Markdown 报告到 `report/repro_report.md`。
报告生成时会检查 Evidence artifact 相对链接，并对 `F:/...`、`D:/...` 这类本机绝对路径做最小脱敏。

验证结果面板也可导出 Repro Pack 到：

```text
artifacts/repro_pack/
artifacts/repro_pack_result.json
```

底部 `testgrid 结果` 面板的“运行门禁”会先运行 `draw_smoke`，通过后解析当前 Case 的 verification summary，并写入：

```text
logs/testgrid_gate.stdout.log
logs/testgrid_gate.stderr.log
artifacts/testgrid_result.json
```

`config/workbench.local.yaml` 或当前 Case 的 `verification.testgrid_plan` 可配置最小 testgrid 计划：

```json
{
  "testgrid_root": "",
  "testgrid_executable": "",
  "testgrid_arguments": "{group} {grid} {case}",
  "testgrid_group": "",
  "testgrid_grid": "",
  "testgrid_case": "",
  "testdiff_executable": "",
  "testdiff_arguments": "--group {group} --grid {grid} --case {case} --out {output}",
  "testdiff_output_root": ""
}
```

未配置 `testgrid_executable` 时，UI 只执行 `draw_smoke` 门禁并解析 `verification/testgrid_summary.txt` / `verification/testdiff_summary.txt`；配置后会在门禁通过后运行指定命令，并额外保存 `logs/testgrid.stdout.log` 和 `logs/testgrid.stderr.log`。

`Run testdiff` 会同样先运行 `draw_smoke` 门禁；门禁通过后执行 `testdiff_executable`，将 `{output}` 指向 `testdiff_output_root` 或当前 Case 的 `artifacts/testdiff_runner_output`，再把 runner 输出中的 `before/after/diff` 目录导入：

```text
logs/testdiff_runner.stdout.log
logs/testdiff_runner.stderr.log
verification/testdiff_summary.txt
artifacts/testdiff_adapter_result.json
artifacts/testdiff_adapter_manifest.json
artifacts/testdiff/before/
artifacts/testdiff/after/
artifacts/testdiff/diff/
```

源码页支持本地关键词搜索并跳转到命中文件行。右侧相似案例面板可按关键词或当前诊断重排，诊断面板可导出：

```text
report/diagnosis_report.md
```

候选补丁面板可记录人工审查状态，并导出最小审查报告：

```text
report/patch_review.md
```

左侧 Case 面板支持：

```text
新建 Case
打开包含 case.json 的 Case 目录
保存当前 Case manifest
刷新 cases/ 列表
双击列表项切换当前 Case
```

## 下一步

见 `doc/roadmap2.md`。`doc/roadmap1.md` 仅保留为历史参考，不再作为后续开发依据。
