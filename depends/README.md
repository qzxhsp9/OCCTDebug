# depends

本目录用于记录**如何获取** OCCT 与 Qt6，而不是提交大型预编译包。

## Qt6

从 [Qt 在线/离线安装器](https://www.qt.io/download) 安装，构建时通过 `CMAKE_PREFIX_PATH` 指向 Qt 安装根目录（含 `lib/cmake/Qt6`）。

## OCCT

从 [OCCT 发布页](https://dev.opencascade.org/release) 获取源码或预编译包，自行编译安装后设置：

- `OpenCASCADE_DIR`：指向安装目录中的 `cmake` 子目录，或
- 将 OCCT 的 `cmake` 路径加入 `CMAKE_PREFIX_PATH`

可选：在此目录添加 `vcpkg.json`、`conanfile.txt` 等锁文件，便于团队统一依赖版本（随项目迭代补充）。
