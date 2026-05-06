# OCCTDebug

用于调试与分析 [Open CASCADE Technology (OCCT)](https://dev.opencascade.org/) 源码的桌面工具。项目以迭代方式演进，长期目标是：在给出问题环境（场景、版本、复现步骤等）后，辅助定位根因。

## 技术栈

- **Qt6**：界面与跨平台桌面壳
- **OCCT**：几何内核与调试对象

## 目录结构

| 路径 | 说明 |
|------|------|
| `src/app/`、`src/ui/`、`src/core/`、`src/occt/`、`src/diagnose/`、`src/io/` | 按开发计划划分的源码模块 |
| `tests/` | CTest：`occtdebug_shape_smoke`（无 Qt，验证 OCCT 与 Shape 树构建） |
| `knowledge/` | 规则/案例/API 占位（里程碑 6） |
| `cmake/` | CMake 辅助脚本（查找依赖、工具链等） |
| `depends/` | 预置 OCCT / FreeType 布局（`depends/occt`、`depends/occt_3rdparty/...`） |
| `doc/` | `OCCTDebug_Development_Plan.md`、`architecture.md`、`roadmap.md` 等 |

## 依赖

- CMake 3.20+
- 支持 C++17 的编译器（MSVC 2019+、GCC 10+、Clang 12+ 等）
- Qt6（Core、Gui、Widgets）
- **OCCT**：按 `cmake/occt_setup_install.cmake` 从 `depends/occt` 解析头文件与库
- **FreeType**：按 `cmake/occt_3rdpart_setup_install.cmake` 从 `depends/occt_3rdparty\freetype-2.13.3-x64` 解析

### Qt 路径

配置时会**先用当前的 `CMAKE_PREFIX_PATH` / `Qt6_DIR` 查找 Qt6**（适合已在 Visual Studio「CMake 变量」或 `CMakeSettings.json` 里配置过 Qt 的情况）。若仍未找到，再使用下面任一方式：

1. 配置 CMake 时设置 **`OCCTDEBUG_QT_ROOT`** 指向含 `lib/cmake/Qt6` 的 Qt 安装根目录，或  
2. 将 `src/QtWorkbenchDefaults.cmake.example` 复制为 **`src/QtWorkbenchDefaults.cmake`**（已 gitignore），在其中设置 **`OCCTDEBUG_QT_DEFAULT_KIT`**。

Windows 下构建完成后会尽量运行 **`windeployqt`**（或回退为复制 `bin` 与 `plugins/platforms`），并把 **OCCT / FreeType** 的 DLL 拷到可执行文件目录。

## 构建

单配置生成器需指定 **`CMAKE_BUILD_TYPE`**，以便选中 `depends/occt` 下对应 Debug/Release 库目录。

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Windows 多配置（Visual Studio）示例：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DOCCTDEBUG_QT_ROOT="C:/Qt/6.11.0/msvc2022_64"
cmake --build build --config Release
```

## 许可证

在确定分发策略前，默认仅用于个人/内部调试；若引用 OCCT/Qt，请遵守各自许可证。
