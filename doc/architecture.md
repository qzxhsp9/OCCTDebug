# OCCTDebug 架构（摘要）

本仓库实现 `doc/OCCTDebug_Development_Plan.md` 中的分层结构，当前代码对应关系如下。

## 分层

| 层次 | 目录 | 职责 |
|------|------|------|
| App / UI | `src/app/`, `src/ui/` | 主窗口、Shape 树、属性、诊断列表；`ViewerWidget`（Windows AIS/V3d）；`TopologyDetailDock`（Face 参数域 UV、`EdgeSchematicWidget`、Vertex 邻边列表） |
| Debug Session（部分） | `core/ProblemContext.h` | 问题元数据、构建信息；完整会话见里程碑 4 |
| Data Capture | `src/occt/ShapeInspector.*` | 从 `TopoDS_Shape` 构建 `ShapeDocument` 与拓扑树元数据 |
| Diagnostic Engine | `src/diagnose/` | 规则接口、注册表、引擎调度 |
| IO | `src/io/` | BREP/STEP 加载、Markdown 报告、Shape 树 JSON、`.occtdbg`（`SessionSerializer`）、最小复现目录（`ReproPackageExporter`） |
| Knowledge | `knowledge/`（占位） | 规则/案例/API 知识库，里程碑 6 扩展 |

## 数据流

1. `BRepLoader` 读入 `TopoDS_Shape`。
2. `ShapeInspector::BuildFromShape` 填充 `ShapeDocument`（扁平节点 + `children` / `parentId`）。
3. UI 绑定 `ShapeDocument`；`DiagnosticEngine` 对 `ProblemContext` + `ShapeDocument` 运行已注册规则。
4. `MarkdownReportExporter` / `ShapeTreeJsonExporter` / `SessionSerializer` / `ReproPackageExporter` 导出结果、会话与复现包。

## 依赖

- Qt6：Widgets、JSON（`ShapeTreeJsonExporter`）。
- OCCT：建模与检查（`BRepTools`、`BRepCheck_Analyzer`、`BRep_Tool`、`TopExp` 等），通过 `depends/occt` 与 `cmake/occt_setup_install.cmake` 链接。

更细的里程碑与路线图见 `doc/roadmap.md`。
