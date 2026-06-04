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
| `src/core/Logger.*` | 最小日志工具 |
| `tests/` | 最小 OCCT 链接 smoke test |
| `cmake/` | OCCT / FreeType 查找脚本 |
| `depends/` | 本地 OCCT / FreeType 依赖 |
| `doc/` | 新产品和 UI 设计文档 |

旧 GUI、旧 Shape 树、旧诊断规则、旧会话和旧导入导出代码已删除；需要时从 git 历史查找。

## 依赖

- CMake 3.20+
- 支持 C++17 的 MSVC / GCC / Clang
- Qt6 Core / Gui / Widgets
- OCCT，本项目按 `cmake/occt_setup_install.cmake` 从 `depends/occt` 解析
- FreeType，本项目按 `cmake/occt_3rdpart_setup_install.cmake` 从 `depends/occt_3rdparty` 解析

## 构建

示例：

```powershell
cmake --preset Qt-Debug
cmake --build out/build/debug --config Debug
ctest --test-dir out/build/debug --output-on-failure
```

如果直接使用 Visual Studio Developer Command Prompt：

```powershell
cmake -S . -B out/build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build/debug --config Debug
ctest --test-dir out/build/debug --output-on-failure
```

## 下一步

见 `doc/roadmap2.md`。`doc/roadmap1.md` 仅保留为历史参考，不再作为后续开发依据。
