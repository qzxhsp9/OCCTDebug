# OCCTDebug 路线图

与 `OCCTDebug_Development_Plan.md` 第 15 节里程碑对齐的进度说明（维护时请更新本文件）。

| 里程碑 | 状态 | 说明 |
|--------|------|------|
| M0 工程骨架 | **初版完成** | 模块化 `src/`、`Logger`、CTest smoke、`doc/architecture.md` / `roadmap.md`、`knowledge/` 占位 |
| M1 Shape 加载与拓扑树 | **进行中** | BREP + **STEP（.stp/.step）**；属性面板 **Face/Edge/Vertex 几何摘要**；JSON 导出 |
| M2 基础诊断规则 | **初版完成** | `DiagnosticEngine`、`RuleRegistry`、R001/R101/R102/R301/R401/R402、诊断面板、Markdown 导出 |
| M3 可视化 Viewer | **进行中** | 主视图：AIS + 线框高亮 + BBox；滚轮 Scale；**resize 防抖 + 擦除节流** 抑闪烁；**TopologyDetailDock**（Face UV/pcurve、Edge/Vertex MVP） |
| M4 问题会话 `.occtdbg` | **初版进行中** | 会话打开/保存 + **Minimal repro folder**（`case/<模型文件>` + `debug.occtdbg` + README）；待：`operations` 建模 |
| M5–M7 | 未开始 | 专题插件、知识库、智能助手 |

## 近期任务

1. 完善 M0：可选 `QCommandLineParser`、日志面板、更多单元测试。
2. 完善 M1：IGES、更丰富的几何域/奇异点提示、STEP 读入选项。
3. 完善 M2：诊断结果与 Shape 树 id 更精确关联（如 R402 映射到文档节点）。
4. 完善 M3：子形状显隐；**拓扑细节视窗**（计划文档 M3「后续迭代」）；诊断多选对象包围盒。
5. 完善 M4：`operations` 序列化、会话内多 BREP、复现包内附带 Markdown 报告。
