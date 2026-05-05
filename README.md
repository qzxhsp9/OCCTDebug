# OCCTDebug

用于调试与分析 [Open CASCADE Technology (OCCT)](https://dev.opencascade.org/) 源码的桌面工具。项目以迭代方式演进，长期目标是：在给出问题环境（场景、版本、复现步骤等）后，辅助定位根因。

## 技术栈

- **Qt6**：界面与跨平台桌面壳
- **OCCT**：几何内核与调试对象

## 目录结构

| 路径 | 说明 |
|------|------|
| `src/` | 应用源码与入口 |
| `cmake/` | CMake 辅助脚本（查找依赖、工具链等） |
| `depends/` | 第三方依赖说明与可选脚本（不提交预编译库） |
| `doc/` | 设计文档、使用说明、调试笔记 |

## 依赖

- CMake 3.20+
- 支持 C++17 的编译器（MSVC 2019+、GCC 10+、Clang 12+ 等）
- Qt6（Core、Gui、Widgets）
- 已安装并可通过 `find_package(OpenCASCADE)` 找到的 OCCT

在 Windows 上若 CMake 找不到 OCCT，可将 `OpenCASCADE_DIR` 指向 OCCT 安装目录下的 `cmake` 文件夹（例如 `.../opencascade-7.x.x/cmake`）。

## 构建

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="<Qt6安装前缀>;<OCCT安装前缀可选>"
cmake --build build
```

Windows 示例（按本机路径修改）：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.7.0/msvc2019_64"
cmake --build build --config Release
```

## 许可证

在确定分发策略前，默认仅用于个人/内部调试；若引用 OCCT/Qt，请遵守各自许可证。
