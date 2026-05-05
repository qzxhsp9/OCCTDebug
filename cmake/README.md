# cmake

放置与构建相关的 CMake 片段，例如：

- 自定义 `Find*.cmake` 或 `OCCTDebug*.cmake`
- 工具链、预设、统一编译选项

根目录 `CMakeLists.txt` 已将 `cmake/` 加入 `CMAKE_MODULE_PATH`，可直接 `include(...)` 本目录下的脚本。
