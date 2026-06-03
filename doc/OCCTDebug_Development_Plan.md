# OCCTDebug 开发计划

## 1. 项目定位

OCCTDebug 是一个用于调试、分析和诊断 Open CASCADE Technology（OCCT）源码与几何算法问题的辅助工具。项目的长期定位不是普通几何查看器，而是一个面向 OCCT bug 的可信证据编译器（evidence compiler）和 AI 自动化分析报告流水线。

项目最终目标是：

> 输入一个 OCCT 问题环境，包括模型、OCCT 版本、调用链、算法参数、日志、异常、几何/拓扑对象状态，工具能够自动复现、自动采集证据、执行诊断规则、匹配历史案例、生成可信 bug 分析报告、提出修复候选和验证计划，并沉淀为可复用回归案例。

项目不是追求“AI 凭文字猜测所有问题”，而是让 AI 建立在机器可验证的证据之上：

```text
问题环境
  ↓
可复现会话 / 操作图
  ↓
结构化采集
  ↓
可视化观察
  ↓
规则诊断
  ↓
EvidenceBundle 可信证据包
  ↓
相似案例匹配
  ↓
AI 组织根因候选 + 修复假设 + 验证建议
  ↓
自动报告 / 修复报告 / 回归用例
  ↓
案例沉淀
```

长期目标是 AI 自动化驱动分析，但核心基础必须是：

1. 结构化采集 OCCT 对象状态；
2. 建立可执行诊断规则；
3. 生成可追溯的 EvidenceBundle；
4. 沉淀可复现调试案例；
5. 输出带证据链的分析和修复报告；
6. 用自动验证支撑修复结论。

---

## 2. 核心目标

### 2.1 短期目标

把当前 GUI 工具升级为“可复现 case + 可信证据包 + 报告生成”的基础工具。

短期能力包括：

- 加载 BREP / STEP 模型；
- 展示 Shape 拓扑树；
- 查看 Shape 类型、容差、包围盒、几何类型；
- 执行基础 BRepCheck；
- 检查 Wire / Shell / Edge / Face 的常见问题；
- 输出诊断结果和 EvidenceBundle；
- 导出 Markdown 诊断报告；
- 保存包含输入、诊断结果、选中 Shape 和操作预留字段的 `.occtdbg`；
- 提供 headless 分析入口，支持批量生成报告。

### 2.2 中期目标

构建专题诊断能力和 AI 辅助 triage 能力。

重点支持：

- Topology 诊断；
- Tolerance 诊断；
- Boolean 诊断；
- Projection 诊断；
- Face Classification 诊断；
- HLR 诊断；
- Meshing 诊断；
- Path Planning 诊断；
- 自动归类 bug 类型；
- 生成根因候选排序；
- 匹配历史案例和相关 API 行为；
- 输出验证计划。

### 2.3 长期目标

构建 OCCT bug 自动分析和修复辅助系统。

长期能力包括：

- 输入问题描述和问题环境；
- 自动选择诊断规则；
- 根据症状匹配历史案例；
- 输出根因候选；
- 给出验证步骤；
- 生成修复候选和补丁草案；
- 自动生成最小复现和回归测试；
- 自动执行跨版本、跨编译器、Debug/Release 验证；
- 支持案例沉淀；
- 支持知识库演进；
- 支持智能诊断解释和修复报告。

---

## 3. 总体架构

当前代码已落地 App/UI、Debug Session、Data Capture、Diagnostic Engine、IO 的基础闭环。后续架构主线应围绕 EvidenceBundle、自动化分析报告、知识检索、修复验证展开。下面按“当前实现 + 后续扩展”描述分层：

```text
OCCTDebug
├── App/UI 层
│   ├── 主窗口
│   ├── Shape 浏览器
│   ├── 属性面板
│   ├── 诊断结果面板
│   ├── 3D Viewer
│   ├── 拓扑细节面板
│   └── 菜单与导出入口
│
├── Debug Session 层
│   ├── 问题环境描述
│   ├── 输入文件管理
│   ├── OCCT 版本信息
│   ├── 算法参数（字段已预留）
│   ├── 诊断快照
│   └── 最小复现目录导出
│
├── Data Capture 层
│   ├── Shape 信息采集
│   ├── 拓扑树构建
│   ├── 几何摘要
│   ├── Tolerance / Bounding 信息采集
│   ├── Face UV / pcurve 采样
│   └── STEP 装配结构检查
│
├── Diagnostic Engine 层
│   ├── 诊断规则
│   ├── 检查项调度
│   ├── 严重等级排序
│   └── 验证建议
│
├── Evidence 层（规划中）
│   ├── EvidenceBundle JSON
│   ├── Shape / Topology / Geometry 摘要
│   ├── BRepCheck / Rule Finding
│   ├── Operation Diff
│   ├── Visual Artifact 索引
│   └── Source / Stack / Log 线索
│
├── IO 层
│   ├── BREP / STEP 加载
│   ├── Shape 树 JSON 导出
│   ├── Markdown 报告导出
│   ├── .occtdbg 会话读写
│   └── 最小复现目录导出
│
├── Automation 层（规划中）
│   ├── Headless CLI
│   ├── Batch Analyze
│   ├── Replay Session
│   ├── Problem Document Importer
│   ├── Repro Sandbox
│   ├── Dataflow Trace Collector
│   ├── Cross-Version Compare
│   └── CI / Regression 集成
│
├── Knowledge Base 层
│   ├── 规则库
│   ├── 问题案例库
│   ├── OCCT API 行为说明
│   ├── 常见错误模式
│   ├── 版本差异记录
│   └── 修复记录
│
├── AI Analysis 层（规划中）
│   ├── Evidence-grounded Prompt Builder
│   ├── RAG 检索
│   ├── 根因候选排序
│   ├── 修复候选生成
│   └── 报告生成
│
├── Codex Handoff 层（规划中）
│   ├── Codex 接管包
│   ├── 调试任务说明
│   ├── 可执行复现 / 测试命令
│   ├── 代码修改边界
│   ├── 调试实验记录
│   ├── 验证结果回填
│   └── before / after 证据对比
│
├── Fix Validation 层（规划中）
│   ├── 最小复现用例生成
│   ├── DRAW / C++ Repro 生成
│   ├── Regression Test 生成
│   ├── Patch 验证
│   └── 修复报告
│
└── Plugin 层（规划中）
    ├── Boolean 诊断插件
    ├── Projection 诊断插件
    ├── Topology 诊断插件
    ├── Tolerance 诊断插件
    ├── Meshing 诊断插件
    ├── HLR 诊断插件
    └── Path Planning 诊断插件
```

### 3.1 落地策略

为了让计划可执行，后续实现应遵循以下顺序和边界：

1. **先抽核心分析库，再扩 CLI 和 GUI**
   将会话加载、Shape 构建、规则诊断、EvidenceBundle 生成放入可复用核心模块。GUI 和 headless CLI 都调用同一套核心逻辑，避免报告、批处理和 UI 诊断结果分叉。

2. **先生成稳定 EvidenceBundle，再接 AI**
   AI 报告、Codex 接管、修复验证都以 `evidence.json` 为事实源。没有 evidenceId 支撑的结论只允许作为 open question。

3. **数据流追踪先做非侵入式快照，再做源码级探针**
   第一阶段通过操作前后 Shape 快照、规则结果、日志和导出物做分析；第二阶段再考虑在 OCCT 算法关键点加可开关的 trace hook，避免一开始破坏 OCCT 内核代码。

4. **Codex 接管先做离线接管包，不做实时远程控制**
   初版只生成 `CodexHandoffPackage`，其中包含任务、证据、复现命令、构建测试命令和修改边界。Codex 输出 patch、报告和 before/after 证据后，再由工具回填。

5. **修复自动化先做草案和验证，不做无人审核合入**
   patch 草案必须配套回归用例和验证结果。只有通过自动验证并经人工 reviewer 确认后，才能进入 `knowledge/fixes/`。

6. **知识库从真实 case 开始，不先做大而空的知识系统**
   每条规则、案例、API 行为和修复记录都应来自已复现问题、已验证报告或人工确认结论。

---

## 4. 关键设计原则

### 4.1 先做可观察，再做可诊断

OCCT 问题通常不能仅凭错误日志定位。

常见问题包括：

- 布尔失败；
- 投影异常；
- Face 分类错误；
- Shell 不闭合；
- Solid 构造异常；
- Edge tolerance 异常；
- SameParameter 不一致；
- pcurve 缺失；
- 退化边处理错误；
- HLR 隐藏线/轮廓线异常；
- Meshing 结果异常。

这些问题都需要观察大量结构化信息：

```text
Shape 类型
拓扑层级
几何类型
Tolerance
Bounding Box
曲线参数域
曲面参数域
Wire 是否闭合
Shell 是否闭合
Edge 是否 SameParameter
Face 是否存在内环
Solid 是否有效
Boolean 输入/输出差异
```

因此，第一阶段重点不是“智能诊断”，而是：

> 把 OCCT 对象状态结构化展示出来。

---

### 4.2 诊断结果必须包含证据链

每个诊断结果不应只输出一句结论，而应包含：

- 规则编号；
- 严重等级；
- 问题标题；
- 问题描述；
- 证据；
- 可能原因；
- 建议动作；
- 关联 Shape。

建议数据结构：

```cpp
enum class DiagnosticSeverity
{
    Info,
    Warning,
    Error,
    Critical
};

struct DiagnosticFinding
{
    std::string ruleId;
    DiagnosticSeverity severity;
    std::string title;
    std::string description;

    std::vector<int> relatedShapeIds;
    std::vector<std::string> evidence;
    std::vector<std::string> possibleCauses;
    std::vector<std::string> suggestions;
};
```

示例输出：

```text
[Warning] Edge tolerance 异常偏大

证据：
- Edge #35 tolerance = 0.284
- 全局平均 Edge tolerance = 1e-6
- 该 Edge 所属 Face #4，参与 Boolean CUT

可能原因：
- 输入模型存在脏数据
- 前序投影/布尔算法放大了容差
- Edge 的 3D 曲线与 pcurve 不一致

建议：
- 检查 BRepCheck_Analyzer
- 尝试 ShapeFix_Edge / SameParameter
- 单独导出 Edge #35 与 Face #4 复现
```

---

### 4.3 保存调试会话

工具不应只是临时查看器，应支持保存调试会话。

当前已定义 `.occtdbg` 会话文件，规范见 `doc/session_format.md`。开发计划中的示例只保留核心字段，不作为唯一规范来源。

示例：

```json
{
  "format": "occtdbg",
  "version": 1,
  "createdAt": "2026-05-06T12:00:00.000Z",
  "problem": {
    "title": "Boolean cut shell by half-space missing section face",
    "category": "Boolean",
    "description": "...",
    "occtVersion": "7.9.3",
    "compiler": "MSVC",
    "buildType": "Release",
    "parameters": {}
  },
  "inputs": [
    {
      "path": "case/input.brep",
      "type": "brep",
      "role": "primary"
    }
  ],
  "operations": [],
  "ui": {
    "selectedShapeId": -1
  },
  "diagnostics": []
}
```

说明：`operations` 当前是预留字段，后续用于记录算法步骤；路径解析、最小复现目录和诊断快照格式以 `doc/session_format.md` 为准。

会话系统当前已支撑部分能力，并为后续扩展提供入口：

- 已支持重新打开问题；
- 已支持导出最小复现；
- 已支持保存诊断快照；
- 后续支持复跑诊断；
- 后续支持积累案例；
- 后续支持对比不同 OCCT 版本结果。

---

## 5. 当前目录结构与后续补齐

当前仓库已有的基线结构：

```text
OCCTDebug/
├── CMakeLists.txt
├── cmake/
├── data/
│   ├── brep/
│   ├── step/
│   └── occtdbg/
├── depends/
├── doc/
├── knowledge/
├── tests/
└── src/
```

其中 `src/` 已拆分为 `app/`、`core/`、`diagnose/`、`io/`、`occt/`、`ui/`，并已有会话、报告、复现包、基础诊断规则和可视化相关实现。当前应按真实结构维护，不再把尚未创建的 `plugins/`、`examples/` 或未拆分出的 Inspector 类写成已存在目录。

```text
OCCTDebug/
├── CMakeLists.txt
├── cmake/
├── depends/
├── doc/
│   ├── architecture.md
│   ├── session_format.md
│   ├── roadmap.md
│   ├── diagnostic_rules.md      （待创建）
│   ├── plugin_design.md         （待创建）
│   └── case_template.md         （待创建）
│
├── knowledge/
│   ├── rules/
│   ├── cases/
│   └── api/
│
├── src/
│   ├── app/
│   │   ├── main.cpp
│   │   ├── MainWindow.h
│   │   └── MainWindow.cpp
│   │
│   ├── core/
│   │   ├── DebugSession.h
│   │   ├── ProblemContext.h
│   │   ├── DiagnosticFinding.h
│   │   ├── ShapeDocument.h
│   │   ├── ShapeDocument.cpp
│   │   ├── ShapeKind.h / ShapeKind.cpp
│   │   ├── Logger.h / Logger.cpp
│   │   └── ShapeNode.h
│   │
│   ├── occt/
│   │   ├── ShapeInspector.h
│   │   ├── ShapeInspector.cpp
│   │   ├── GeometrySummary.h / GeometrySummary.cpp
│   │   ├── FaceUvExtractor.h / FaceUvExtractor.cpp
│   │   ├── BBoxWire.h / BBoxWire.cpp
│   │   └── OcctUtils.h
│   │
│   ├── diagnose/
│   │   ├── IDiagnosticRule.h
│   │   ├── DiagnosticEngine.h
│   │   ├── DiagnosticEngine.cpp
│   │   ├── RuleRegistry.h
│   │   ├── RuleRegistry.cpp
│   │   └── rules/
│   │       ├── CheckNullShapeRule.h
│   │       ├── CheckWireClosedRule.h
│   │       ├── CheckShellClosedRule.h
│   │       ├── CheckToleranceRule.h
│   │       ├── CheckBRepValidityRule.h
│   │       └── CheckSameParameterRule.h
│   │
│   ├── io/
│   │   ├── BRepLoader.h
│   │   ├── BRepLoader.cpp
│   │   ├── SessionSerializer.h
│   │   ├── SessionSerializer.cpp
│   │   ├── MarkdownReportExporter.h
│   │   ├── MarkdownReportExporter.cpp
│   │   ├── ShapeTreeJsonExporter.h / ShapeTreeJsonExporter.cpp
│   │   └── ReproPackageExporter.h / ReproPackageExporter.cpp
│   │
│   ├── ui/
│   │   ├── ShapeTreeWidget.h
│   │   ├── ShapeTreeWidget.cpp
│   │   ├── PropertyPanel.h
│   │   ├── PropertyPanel.cpp
│   │   ├── DiagnosticPanel.h
│   │   ├── DiagnosticPanel.cpp
│   │   ├── ViewerWidget.h
│   │   ├── ViewerWidget.cpp
│   │   ├── TopologyDetailPanel.h / TopologyDetailPanel.cpp
│   │   ├── FaceUvCanvasWidget.h / FaceUvCanvasWidget.cpp
│   │   └── EdgeSchematicWidget.h / EdgeSchematicWidget.cpp
│
├── tests/
│   ├── CMakeLists.txt
│   └── shape_smoke.cpp
│
└── data/
    ├── brep/
    ├── step/
    └── occtdbg/
```

后续如引入插件系统或案例库，再创建 `src/plugins/`、`data/cases/` 或 `examples/`。在那之前，文档中的插件设计只作为路线图，不应暗示已有实现。

---

## 6. 核心数据模型设计

### 6.1 Shape 类型

```cpp
enum class ShapeKind
{
    Unknown,
    Compound,
    CompSolid,
    Solid,
    Shell,
    Face,
    Wire,
    Edge,
    Vertex
};
```

### 6.2 Shape 节点

```cpp
struct ShapeNode
{
    int id = -1;
    int parentId = -1;

    ShapeKind kind = ShapeKind::Unknown;
    std::string name;

    TopoDS_Shape shape;

    double tolerance = 0.0;

    bool isNull = false;
    bool isClosed = false;
    bool isValid = true;

    Bnd_Box bbox;

    std::vector<int> children;
};
```

### 6.3 Shape 文档

```cpp
class ShapeDocument
{
public:
    int AddNode(const ShapeNode& node);

    const ShapeNode* FindNode(int id) const;
    ShapeNode* FindNode(int id);

    const std::vector<ShapeNode>& Nodes() const;

    TopoDS_Shape RootShape() const;
    void SetRootShape(const TopoDS_Shape& shape);

private:
    TopoDS_Shape m_rootShape;
    std::vector<ShapeNode> m_nodes;
};
```

### 6.4 问题类型

```cpp
enum class ProblemCategory
{
    Unknown,
    Boolean,
    Projection,
    Classification,
    Topology,
    Tolerance,
    Meshing,
    HLR,
    Performance,
    Crash
};
```

### 6.5 问题上下文

```cpp
struct ProblemContext
{
    std::string title;
    ProblemCategory category = ProblemCategory::Unknown;
    std::string description;
    std::string expectedBehavior;
    std::string actualBehavior;

    std::string occtVersion;
    std::string compiler;
    std::string buildType;

    std::vector<std::string> inputFiles;
    std::map<std::string, std::string> parameters;
};
```

当前代码中的 `ProblemContext` 已包含 `expectedBehavior` 和 `actualBehavior`，并已接入 `.occtdbg` 保存/读取、问题 Markdown 导入和诊断 Markdown 报告导出。旧 `.occtdbg` 可缺省这两个字段，读取时按空字符串处理。

### 6.6 BugCase / EvidenceBundle / CodexHandoff（规划）

为支撑可信证据和 Codex 接管，需要在现有 `DebugSession` 之上增加三类规划模型：

```text
BugCase
  problem: ProblemContext
  inputs: SessionInput[]
  operations: OperationStep[]
  evidenceBundlePath
  reports[]

EvidenceBundle
  caseId
  environment
  inputs
  operationGraph
  shapeSummary
  diagnosticFindings
  artifacts
  similarCases
  validation

CodexHandoffPackage
  task
  guardrails
  evidenceBundle
  reproSandbox
  commands
  sourceContext
  expectedOutputs
```

实现策略：

- `DebugSession` 继续保持轻量，负责可打开、可保存、可 replay；
- `BugCase` 作为更高层的问题容器，汇总问题文档、会话、证据、报告和验证结果；
- `EvidenceBundle` 作为 AI 和报告的唯一事实源；
- `CodexHandoffPackage` 作为离线交接物，不直接耦合 GUI。

---

## 7. 诊断引擎设计

### 7.1 诊断规则接口

每个诊断规则独立实现。

```cpp
class IDiagnosticRule
{
public:
    virtual ~IDiagnosticRule() = default;

    virtual std::string Id() const = 0;
    virtual std::string Name() const = 0;
    virtual ProblemCategory Category() const = 0;

    virtual bool IsApplicable(
        const ProblemContext& context,
        const ShapeDocument& document) const = 0;

    virtual std::vector<DiagnosticFinding> Run(
        const ProblemContext& context,
        const ShapeDocument& document) const = 0;
};
```

### 7.2 规则注册器

```cpp
class RuleRegistry
{
public:
    void Register(std::unique_ptr<IDiagnosticRule> rule);

    std::vector<const IDiagnosticRule*> GetApplicableRules(
        const ProblemContext& context,
        const ShapeDocument& document) const;

private:
    std::vector<std::unique_ptr<IDiagnosticRule>> m_rules;
};
```

### 7.3 诊断引擎

```cpp
class DiagnosticEngine
{
public:
    std::vector<DiagnosticFinding> Diagnose(
        const ProblemContext& context,
        const ShapeDocument& document);

private:
    RuleRegistry m_registry;
};
```

诊断流程：

```text
ProblemContext + ShapeDocument
  ↓
RuleRegistry 筛选适用规则
  ↓
逐个执行规则
  ↓
收集 DiagnosticFinding
  ↓
按严重等级和规则编号排序；未来可在 `DiagnosticFinding` 增加 confidence 后再引入置信度排序
  ↓
输出到 DiagnosticPanel / ReportExporter
```

---

## 8. 诊断规则候选池与当前实现

当前已实现的基础规则为 R001、R101、R102、R301、R401、R402。下面的表包含已实现规则和后续候选规则；未实现规则需要在进入里程碑任务前补充输入、证据、阈值和验收用例。

### 8.1 通用 Shape 规则

| 规则编号 | 规则名称 | 说明 |
|---|---|---|
| R001 | Shape 是否为空 | 检查输入 Shape 是否 Null |
| R002 | Shape 拓扑类型是否符合预期 | 判断输入类型是否和问题上下文匹配 |
| R003 | 是否含多个根对象 | 检查 Compound 中是否包含多个主对象 |
| R004 | Bounding Box 是否异常 | 检查包围盒尺寸是否极大或极小 |
| R005 | 是否存在极小边/极小面 | 检查接近容差级别的几何对象 |

### 8.2 拓扑规则

| 规则编号 | 规则名称 | 说明 |
|---|---|---|
| R101 | Wire 是否闭合 | 检查 Wire 的首尾连接 |
| R102 | Shell 是否闭合 | 检查 Shell 是否可作为封闭体边界 |
| R103 | Face 是否缺少外环 | 检查 Face 外边界 |
| R104 | Face 是否存在多个外环候选 | 检查内外环方向和层级 |
| R105 | Edge 是否被异常数量的 Face 共享 | 检查 non-manifold 情况 |
| R106 | Vertex 容差是否大于相邻 Edge 长度 | 检查容差污染 |

### 8.3 几何规则

| 规则编号 | 规则名称 | 说明 |
|---|---|---|
| R201 | Edge 是否缺少 3D Curve | 检查 BRep_Tool::Curve |
| R202 | Edge 是否缺少 pcurve | 检查 Edge on Face 的 2D 曲线 |
| R203 | Face 是否缺少 Surface | 检查 BRep_Tool::Surface |
| R204 | Curve 参数域是否异常 | 检查 First/Last 参数 |
| R205 | Surface 参数域是否异常 | 检查 UV 参数范围 |
| R206 | Degenerated Edge 是否未正确标记 | 检查退化边状态 |

### 8.4 容差规则

| 规则编号 | 规则名称 | 说明 |
|---|---|---|
| R301 | Tolerance 是否过大 | 检查 Vertex / Edge / Face tolerance |
| R302 | 相邻实体 Tolerance 是否不一致 | 检查容差突变 |
| R303 | Edge 长度是否小于 Tolerance | 检查无效短边 |
| R304 | Face 尺寸是否小于 Tolerance | 检查极小面 |
| R305 | Boolean 前后 Tolerance 是否被放大 | 对比输入输出对象容差 |

### 8.5 BRepCheck 规则

| 规则编号 | 规则名称 | 说明 |
|---|---|---|
| R401 | BRepCheck 是否通过 | 使用 BRepCheck_Analyzer |
| R402 | SameParameter 检查 | 检查 Edge 的 SameParameter |
| R403 | SameRange 检查 | 检查 Edge 的 SameRange |
| R404 | Wire SelfIntersection | 检查 Wire 自交 |
| R405 | InvalidCurveOnSurface | 检查 pcurve 与 surface 不一致 |
| R406 | Shell Orientation 问题 | 检查 Shell 面方向 |

### 8.6 Boolean 专题规则

| 规则编号 | 规则名称 | 说明 |
|---|---|---|
| R501 | Boolean 输入类型检查 | 检查 Solid/Shell/Face 类型是否合理 |
| R502 | Boolean 输入是否 non-manifold | 检查拓扑合法性 |
| R503 | 工具体是否为 HalfSpace | 检查是否使用无限半空间 |
| R504 | Shell Cut HalfSpace 缺少截面 Face | 识别 Sheet Body 裁剪不会自动补面的问题 |
| R505 | Boolean 操作选择建议 | 判断 CUT / COMMON / SECTION 是否更合适 |
| R506 | 是否需要 Section + Split | 针对 sheet 裁剪问题给出建议 |
| R507 | 是否需要 FuzzyValue | 针对轻微不相交或容差问题给出建议 |

---

## 9. 问题环境输入设计

### 9.1 Problem Wizard

当前已实现第一版 Problem Document 流程：用户可以通过 `File -> Create problem document...` 生成标准 `problem.md`，也可以通过 `File -> Import problem document...` 将手写或生成的 `problem.md` 导入为 `ProblemContext`、输入文件、预期行为、实际行为和复现步骤。模板界面已预留自定义属性的新增/删除入口；多个 BREP/STEP 输入会以一个 OCCT compound 载入主界面。主界面顶部应始终标注当前正在分析的问题，工具使用场景按“一个问题的复现、证据采集、诊断和报告”展开，而不是单纯围绕孤立模型浏览展开。后续 Problem Wizard 不应替代 Markdown，而应围绕同一格式继续增强校验、参数模板和 Codex 接管所需的复现信息。

字段包括：

```text
问题类型：
- Boolean 异常
- 投影异常
- 点/Face 分类异常
- Shell/Solid 判断异常
- HLR 显隐线异常
- Meshing 异常
- Crash
- Performance
- Unknown

输入文件：
- 原始模型
- 中间模型
- 结果模型
- 日志
- 调用代码片段

环境：
- OCCT 版本
- Debug/Release
- 编译器
- 操作系统
- 是否启用 FuzzyValue
- 是否做过 ShapeFix

行为：
- Expected Behavior
- Actual Behavior
- Reproduction Steps
- Notes / Suspected Area
```

优先级：

1. 先支持 `problem.md` 导入；
2. 再支持 `.occtdbg` 中持久化 expected/actual/reproduction；
3. 最后再做面向用户的 Problem Wizard UI。

### 9.2 Boolean 问题模板

```json
{
  "category": "Boolean",
  "operation": "CUT",
  "argumentShape": "body.brep",
  "toolShape": "tool.brep",
  "fuzzyValue": 0.0,
  "nonDestructive": true,
  "glue": "Off",
  "expected": "生成带截面 Face 的结果",
  "actual": "只有 shell，没有截面 Face"
}
```

### 9.3 Projection 问题模板

```json
{
  "category": "Projection",
  "sourceFace": "face.brep",
  "targetPlane": {
    "origin": [0, 0, 0],
    "normal": [0, 0, 1],
    "xDir": [1, 0, 0]
  },
  "direction": [0, 0, 1],
  "expected": "获得 2D face",
  "actual": "内环分类错误"
}
```

---

## 10. UI 设计

### 10.1 主界面布局

当前主界面不是固定四象限，而是 `QMainWindow` + splitters + diagnostic dock：

```text
+--------------------------------------------------------------------------------+
| Menu: File / Diagnostics / Export / View / Help                                 |
+-------------------------------+------------------------------------------------+
| Left vertical splitter         | Right vertical splitter                         |
|                               |                                                |
| ShapeTreeWidget                | ViewerWidget (AIS/V3d 3D viewer)                |
|                               |                                                |
| PropertyPanel                  | TopologyDetailPanel                             |
|                               | - FaceUvCanvasWidget                            |
|                               | - EdgeSchematicWidget                           |
+-------------------------------+------------------------------------------------+
| Right dock area: Diagnostic log dock (DiagnosticPanel, toggleable from View)     |
+--------------------------------------------------------------------------------+
```

当前菜单能力：

- File：Open model、Open session、Save session；
- Diagnostics：Run diagnostics、Batch check STEP assemblies；
- Export：Diagnostic report (Markdown)、Shape tree (JSON)、Minimal repro folder；
- View：Fit all、Diagnostic log dock、Show bounding box；
- Help：Mouse controls。

### 10.2 当前基础 UI 功能

- 已支持打开 BREP / STEP；
- 已支持 Shape 树展示；
- 已支持属性面板展示；
- 已支持诊断结果列表；
- 已支持点击诊断结果定位 Shape；
- 已支持导出 Markdown 报告；
- 已支持导出 Shape 树 JSON；
- 已支持保存 / 打开 `.occtdbg` 会话；
- 已支持导出最小复现目录。

### 10.3 已落地的可视化 UI

- 3D Viewer；
- ShapeTree / DiagnosticPanel 选择高亮；
- Bounding Box 显示开关；
- 基础视角控制：旋转、平移、缩放、Fit all；
- TopologyDetailPanel：Face UV/pcurve 折线、Edge 容差示意、Vertex 邻接 Edge 列表。

### 10.4 后续 UI 缺口

- 子形状隐藏 / 显示；
- 诊断多选对象包围盒；
- 异常对象聚合视图；
- Problem Wizard / 专题参数输入；
- 日志面板或调用链面板；
- Boolean 前后对比；
- Section 线显示；
- 插件系统 UI。

---

## 11. 可视化能力规划

### 11.1 已落地

```text
显示整个 Shape
选择 ShapeTree 节点后高亮对应 Shape
显示 Bounding Box
显示 Face / Edge / Vertex 基本属性
显示 Face UV / pcurve 布局
显示 Edge 容差示意
显示 Vertex 邻接 Edge 列表
```

### 11.2 下一阶段

```text
显示异常 Shape
诊断结果多选定位
显示 tolerance 热力图
强化 Wire 方向、Face 外环 / 内环、Edge 参数方向显示
子形状隐藏 / 显示
显示 Section 线
显示 Boolean 前后差异
```

### 11.3 后期

```text
显示诊断证据链
显示算法过程帧
显示中间结果对比
显示 Shape 演化历史
支持多版本 OCCT 结果对比
```

---

## 12. EvidenceBundle 与报告系统设计

自动化 bug 分析和修复报告必须从 EvidenceBundle 生成。Markdown 报告只是最终呈现形式，不能成为唯一数据源。

### 12.1 EvidenceBundle

EvidenceBundle 是一次分析的机器可读证据包，建议使用 JSON 保存，并允许附带图片、Shape 片段、日志和复现脚本。

```json
{
  "caseId": "CASE-2026-001",
  "session": "debug.occtdbg",
  "environment": {
    "occtVersion": "7.9.3",
    "compiler": "MSVC",
    "buildType": "Release",
    "platform": "Windows"
  },
  "inputs": [
    {
      "path": "case/input.brep",
      "type": "brep",
      "role": "primary",
      "sha256": "..."
    }
  ],
  "operationGraph": [],
  "shapeSummary": {},
  "topologySummary": {},
  "geometrySummary": {},
  "toleranceStats": {},
  "brepCheck": {},
  "diagnosticFindings": [],
  "visualArtifacts": [],
  "similarCases": [],
  "sourceSuspects": [],
  "validation": {}
}
```

设计要求：

- 每条结论必须能追溯到 ruleId、shapeId、输入文件、操作参数或日志片段；
- AI 只能引用 EvidenceBundle 中的事实，不能凭空生成不可验证结论；
- 报告、案例库、RAG prompt、修复验证都应消费同一个 EvidenceBundle；
- EvidenceBundle 可以独立于 GUI 由 headless CLI 生成。

### 12.2 当前报告能力

当前实现由 `MarkdownReportExporter` 生成基础 Markdown，包含环境、问题、输入 Shape 列表、诊断结果和建议下一步。

当前报告模板：

```markdown
# OCCTDebug Diagnostic Report

## 1. Environment

- OCCT Version:
- Build Type:
- Compiler:
- Platform:

## 2. Problem

- Title:
- Description:
- Category:

## 3. Input Shapes

| ID | Type | Tolerance | BBox valid |
|---|---|---|---|

## 4. Diagnostic Findings

### R301 Edge tolerance too large

Severity: Warning

Evidence:
- Edge #35 tolerance = 0.284
- Edge length = 0.31

Suggestions:
- Run ShapeFix_Edge
- Check SameParameter
- Export Edge #35 as minimal case

## 5. Suggested Next Steps

...
```

说明：`DiagnosticFinding` 数据结构已包含 `possibleCauses`，但当前 Markdown 导出尚未输出该字段；如果后续需要面向知识库沉淀，应补齐报告中的 Possible Causes 和 relatedShapeIds。

### 12.3 目标报告结构

面向 OCCT bug 分析和修复，最终报告应固定为以下结构：

```markdown
# OCCT Bug Analysis Report

## Summary

## Environment

## Reproduction

## Observed Behavior

## Expected Behavior

## Evidence

## Root Cause Hypotheses

## Similar Cases

## Source Suspects

## Recommended Fix

## Validation Plan

## Regression Artifacts
```

其中：

- `Evidence` 只引用 EvidenceBundle；
- `Root Cause Hypotheses` 必须包含证据、置信度、反证条件；
- `Recommended Fix` 必须说明风险和影响模块；
- `Validation Plan` 必须包含最小复现、回归测试、跨版本对比和性能风险；
- `Regression Artifacts` 记录生成的 `.brep`、`.occtdbg`、DRAW script、C++ repro 或测试文件。

---

## 13. 知识库设计

知识库是项目能否自动生成可信分析和修复建议的关键。知识库不应只是文本文档，而应是规则、案例、API 行为、修复记录和验证结果的结构化集合。

OCCT bug 与版本差异的采集、整理和审核由独立模块 **OCCT Bug Knowledge Collector** 承担，详见 `doc/occt_bug_knowledge_collector.md`。该模块负责从官方 GitHub Issues/Releases、OCCT 文档、upgrade notes、历史 archive、论坛线索、手动录入和当前 EvidenceBundle 中生成知识条目，并提供可信度标注和人工审核流程。

建议分为五类：

```text
knowledge/
├── rules/
├── cases/
├── api/
├── fixes/
├── commits/
└── items/
```

### 13.1 Rule Knowledge

规则知识使用 YAML 维护。

示例：

```yaml
id: R504
title: Shell cut by half-space missing section face
category: Boolean
symptoms:
  - input_shape_is_shell
  - tool_shape_is_halfspace
  - operation_is_cut
  - result_has_no_section_face
possible_causes:
  - OCCT Boolean cut on sheet body does not automatically cap section
suggestions:
  - use section + split
  - construct solid before cut
  - explicitly build section wire and face
confidence: 0.85
```

### 13.2 Case Knowledge

历史案例使用 JSON 维护，并保存症状签名、最小复现、根因、修复方式和验证结果。

示例：

```json
{
  "caseId": "BOOLEAN_HALFSPACE_SHELL_001",
  "title": "Shell cut by half-space no cap face",
  "occtVersions": ["7.7", "7.8", "7.9"],
  "symptoms": [
    "input shape is shell",
    "tool shape is half-space",
    "result has no cap face"
  ],
  "rootCause": "Boolean cut on sheet body does not automatically generate section cap face.",
  "solution": "Use Section + Split, or explicitly build section wire and face.",
  "relatedRules": ["R504"],
  "evidenceBundle": "evidence/BOOLEAN_HALFSPACE_SHELL_001.json",
  "regression": {
    "repro": "cases/BOOLEAN_HALFSPACE_SHELL_001/debug.occtdbg",
    "test": "tests/bugs/boolean_halfspace_shell_001"
  }
}
```

### 13.3 API Knowledge

记录 OCCT API 行为。

示例：

```yaml
api: BRepAlgoAPI_Cut
notes:
  - solid vs solid cut may produce capped volume
  - shell vs solid cut usually remains shell-like result
  - section face is not always automatically generated
related:
  - BRepAlgoAPI_Section
  - BOPAlgo_Splitter
  - BRepBuilderAPI_MakeFace
```

### 13.4 Fix Knowledge

修复知识记录“什么症状最终对应什么代码修改和回归验证”。

```json
{
  "fixId": "FIX_BOOLEAN_001",
  "relatedCases": ["BOOLEAN_HALFSPACE_SHELL_001"],
  "affectedModules": ["BOPAlgo", "BRepAlgoAPI"],
  "patchSummary": "Explicitly preserve section wire for sheet-body half-space cut workflow.",
  "risk": ["Boolean behavior compatibility", "Tolerance expansion"],
  "validation": [
    "minimal repro passes",
    "existing Boolean regression suite passes",
    "no performance regression on corpus"
  ]
}
```

---

## 14. AI 自动化分析接入规划

AI 是分析和报告编排层，不是 OCCT 几何判定层。AI 必须由 EvidenceBundle、规则结果、知识库和验证结果驱动。

推荐架构：

```text
Session / Input / OperationGraph
        ↓
Headless Analyzer 生成 EvidenceBundle
        ↓
Diagnostic Engine 输出结构化 findings
        ↓
Knowledge Retrieval 检索规则/案例/API/修复记录
        ↓
AI Reasoner 生成根因候选、修复假设、验证计划
        ↓
Report Generator 生成 bug 分析报告
        ↓
Fix Assistant 生成 patch 草案 / regression test 草案
        ↓
Validation Runner 验证
        ↓
用户确认结果是否正确
        ↓
沉淀新案例 / 修复知识
```

AI 适合做：

- 组织解释；
- 匹配相似案例；
- 生成调试步骤；
- 总结报告；
- 根据证据排序根因候选；
- 生成修复候选；
- 生成回归测试草案；
- 生成 reviewer 可读的风险分析。

AI 不应直接做：

- 直接判断 TopoDS_Shape；
- 直接替代 OCCT 检查；
- 直接猜测没有证据的根因；
- 直接生成不可验证结论；
- 未经测试直接修改几何内核行为。

AI 输出必须遵守：

- 每个结论引用 EvidenceBundle 中的 evidenceId；
- 每个根因假设包含 confidence 和反证条件；
- 每个修复建议包含验证步骤；
- patch 草案必须同时生成或引用回归用例；
- 修复报告必须包含 before/after 结果。

### 14.1 Codex 接管模式

AI 自动化分析需要预留一种“交给 Codex 接管”的工作模式。该模式不是让 Codex 直接凭描述猜 bug，而是由 OCCTDebug 生成一个完整、受控、可执行的接管包，让 Codex 在明确边界内完成调试、分析、修复尝试和测试验证。

Codex 接管模式的输入：

```text
CodexHandoffPackage
├── debug.occtdbg
├── evidence.json
├── analysis_report.md
├── repro/
│   ├── input.brep / input.step
│   ├── README.txt
│   └── optional_draw_script.tcl
├── commands/
│   ├── reproduce.ps1
│   ├── analyze.ps1
│   ├── build.ps1
│   └── test.ps1
├── source_context.json
├── task.md
└── guardrails.md
```

其中：

- `task.md` 描述 Codex 需要完成的目标，例如“定位 R402 SameParameter 失败原因并生成修复建议”；
- `guardrails.md` 描述允许修改的目录、禁止操作、测试必须通过的命令、报告格式；
- `source_context.json` 记录疑似源码模块、相关调用栈、历史案例、可能影响范围；
- `commands/` 中所有命令必须可复制执行，避免 Codex 依赖隐含环境；
- `evidence.json` 是 Codex 推理和报告的事实来源。

Codex 接管后的职责：

- 复现问题；
- 阅读 EvidenceBundle 和源码；
- 补充必要的日志或探针；
- 定位疑似根因；
- 生成 patch 草案或修复建议；
- 新增或更新回归测试；
- 运行指定构建和测试命令；
- 输出 before/after EvidenceBundle 差异；
- 生成 Codex 调试报告。

Codex 接管模式的输出：

```text
codex_result/
├── codex_debug_report.md
├── patch.diff
├── regression_tests/
├── before_evidence.json
├── after_evidence.json
├── validation_report.md
└── open_questions.md
```

安全边界：

- Codex 不能绕过 EvidenceBundle 给出根因结论；
- Codex 不能把未验证 patch 标记为 recommended fix；
- Codex 必须记录执行过的命令和关键输出；
- Codex 修改必须可 diff、可回滚、可由人工 reviewer 审核；
- 如果复现失败，输出应转为“复现失败分析”，而不是继续猜测。

### 14.2 OCCT 能力成长与可信度分级

现阶段，Codex 对 OCCT 的理解应按证据和案例逐步提升，而不是默认具备完整 OCCT 内核维护能力。项目需要把“学习 OCCT”工程化，使每次交互、分析和修复都能沉淀为可检索知识。

能力分级：

| 等级 | 能力边界 | 可允许输出 |
|---|---|---|
| L0 通用工程理解 | C++ / Qt / CMake / 基础调试能力 | 工程结构建议、构建问题分析、文档整理 |
| L1 OCCT API 辅助理解 | 基于 OCCT 文档、源码、已有规则理解 API 行为 | API 使用建议、可疑模块定位、待验证假设 |
| L2 案例验证理解 | 基于已复现 case、EvidenceBundle、历史案例和测试结果 | 根因候选排序、验证计划、回归用例草案 |
| L3 修复验证理解 | 基于 before/after EvidenceBundle、patch、回归测试和 reviewer 反馈 | patch 草案、修复报告、知识库沉淀 |
| L4 维护者级能力目标 | 长期积累大量 OCCT 模块案例、源码变更和回归结果 | 可直接修改 OCCT 代码，但仍必须通过自动验证和人工 review |

学习闭环：

```text
问题文档 / 复现模型
  ↓
EvidenceBundle
  ↓
Codex 分析与实验记录
  ↓
人工确认 / reviewer 反馈
  ↓
知识库条目：rules / cases / api / fixes
  ↓
后续相似问题检索与 prompt 上下文
```

要求：

- 未达到 L2 的模块，Codex 只能输出建设性意见和待验证假设；
- 未达到 L3 的问题，Codex 不能把 patch 标为 recommended fix；
- 每次被人工确认的结论都要转成知识库条目；
- 每次失败的分析也要记录原因，避免后续重复错误路径；
- OCCT 源码修改能力必须由 case 数量、回归测试覆盖和 reviewer 反馈逐步校准。

### 14.3 调试分析环境设计

目标调试环境应支持用户手动编写问题文档，然后选择“Codex 接管分析”。工具负责把问题文档、模型、会话、复现命令、数据流证据和源码上下文组装成一个可执行分析环境。

输入形态：

```text
problem.md
models/
  input.brep
  tool.step
logs/
  crash.log
  callstack.txt
debug.occtdbg（可选）
```

`problem.md` 推荐结构：

```markdown
# Problem

## Summary

## Environment

## Input Files

## Reproduction Steps

## Expected Behavior

## Actual Behavior

## Notes / Suspected Area
```

工具需要开发的调试分析环境组件：

| 组件 | 作用 |
|---|---|
| Problem Document Importer | 将 `problem.md` 解析为 `ProblemContext`、输入文件列表、预期/实际行为和复现步骤 |
| Operation Graph Builder | 将用户步骤、GUI 操作或脚本转成可 replay 的 `operations` |
| Repro Sandbox Generator | 生成隔离复现目录，固定模型、会话、命令和环境说明 |
| Command Harness | 生成并执行 `reproduce`、`analyze`、`build`、`test` 等命令 |
| Dataflow Trace Collector | 在关键算法节点导出输入/中间/输出 Shape、参数、容差、BRepCheck、异常 |
| Shape Snapshot / Diff | 对比操作前后拓扑、几何、容差、BBox、pcurve、SameParameter |
| Source Context Indexer | 建立 OCCT 源码符号、文件、调用点、测试用例和历史修复索引 |
| Experiment Ledger | 记录 Codex 每次假设、命令、观察、失败路径和结论 |
| Validation Runner | 运行回归用例、构建命令、跨配置测试并输出验证结果 |
| Result Ingestor | 将 Codex 输出的报告、patch、测试结果、before/after EvidenceBundle 回填到 case |

Codex 接管分析流程：

```text
用户提供 problem.md + 模型 / 日志
  ↓
OCCTDebug 导入 ProblemContext
  ↓
生成或补全 debug.occtdbg / operations
  ↓
创建 Repro Sandbox
  ↓
运行 Headless Analyzer 生成 EvidenceBundle
  ↓
生成 CodexHandoffPackage
  ↓
Codex 复现问题并读取数据流证据
  ↓
Codex 提出假设、加探针、运行实验
  ↓
Codex 生成 patch 草案或修复建议
  ↓
Validation Runner 运行测试
  ↓
回填 before/after EvidenceBundle 和 Codex 调试报告
  ↓
人工确认后沉淀到 knowledge/
```

这个环境的关键不是让 Codex “看一份文档后直接回答”，而是让 Codex 能够：

- 复现用户问题；
- 访问结构化数据流；
- 读取源码和历史案例；
- 不断提出假设、实验、反证；
- 用构建和测试验证结论；
- 最终输出可审查的分析报告和修复报告。

---

## 15. 里程碑规划

本节描述当前基线和后续缺口。更短的进度摘要见 `doc/roadmap.md`，维护时两处需要同步。

| 里程碑 | 当前状态 | 主要剩余缺口 |
|---|---|---|
| M0 工程骨架 | 初版完成 | 日志面板、更多单元测试、命令行入口可选增强 |
| M1 Shape 加载与拓扑树 | 进行中 | IGES、更丰富的几何域/奇异点提示、STEP 读入选项 |
| M2 基础诊断规则 | 初版完成 | 诊断结果与 Shape 节点更精确关联，补充更多规则 |
| M3 可视化 Viewer | 进行中 | 子形状显隐、诊断多选对象包围盒、拓扑细节视窗阶段 2+ |
| M4 问题会话 | 初版进行中 | `operations` 建模、多输入会话、复现包附带 Markdown 报告 |
| M5 EvidenceBundle | 未开始 | 统一证据包 schema、证据 id、报告从证据包生成 |
| M6 Headless Analyzer / Debug Lab | 未开始 | CLI 分析、问题文档导入、复现沙箱、数据流追踪、CI 集成 |
| M7 Knowledge + AI Report | 未开始 | 规则/案例/API/修复知识库，AI 生成可信分析报告 |
| M8 Codex Handoff | 未开始 | 生成 Codex 接管包，支持调试、分析、修改、测试回填 |
| M9 Fix Validation | 未开始 | 修复候选、回归用例、验证报告 |
| M10 专题插件 | 未开始 | Boolean / Projection / Meshing 等专题诊断扩展 |

## Milestone 0：工程骨架稳定（初版完成）

### 目标

整理当前工程结构，为后续开发打基础。

### 任务

- 已整理 `src/` 目录结构；
- 已增加 `core/`、`occt/`、`diagnose/`、`io/`、`ui/` 子模块；
- 已增加基础日志系统；
- 已增加 CTest smoke 测试；
- 已增加 `doc/architecture.md`；
- 已增加 `doc/roadmap.md`；
- 后续补充日志面板、命令行参数和更完整单元测试。

### 验收标准

- 项目可在 Windows + Visual Studio + Qt6 + OCCT 下稳定构建；
- 启动后能显示 OCCT 版本；
- 代码目录结构清晰；
- 后续模块扩展不需要大规模重构。

---

## Milestone 1：Shape 加载与拓扑树（进行中）

### 目标

打开 BREP / STEP 文件并查看拓扑结构。

### 任务

- 已实现 `BRepLoader`，支持 `.brep`、`.stp`、`.step` 基础读入；
- 已实现 `ShapeDocument`；
- 已实现 `ShapeInspector`；
- 已实现 `ShapeTreeWidget`；
- 已支持点击树节点显示 Shape 基础属性；
- 已支持导出 Shape 树为 JSON；
- 后续补充 IGES、STEP 读入选项、装配/属性语义和更丰富的几何域提示。

### 验收标准

- 能打开 `.brep`、`.stp`、`.step`；
- 能显示 Compound / Solid / Shell / Face / Wire / Edge / Vertex 层级；
- 能查看每个 Shape 的类型、容差、包围盒；
- 能导出基础 Shape 信息。

---

## Milestone 2：基础诊断规则（初版完成）

### 目标

形成第一个“诊断闭环”。

### 任务

- 已实现 `IDiagnosticRule`；
- 已实现 `DiagnosticEngine`；
- 已实现 `RuleRegistry`；
- 已实现第一批基础规则；
- 已支持 UI 显示诊断结果；
- 已支持点击诊断结果定位相关 Shape；
- 已支持导出 Markdown 报告；
- 后续重点是提高规则证据质量、补齐 Shape id 映射和增加更多规则。

### 优先规则

- R001 Shape 是否为空；
- R101 Wire 是否闭合；
- R102 Shell 是否闭合；
- R301 Tolerance 是否异常；
- R401 BRepCheck 是否通过；
- R402 SameParameter 检查。

### 验收标准

- 用户打开模型后点击“诊断”；
- 工具输出 Warning / Error；
- 每条诊断结果有证据和建议；
- 诊断结果可以导出为 Markdown。

---

## Milestone 3：可视化 Viewer（进行中）

### 目标

真正辅助调试几何。

### 任务

- 已集成 OCCT `AIS_InteractiveContext`；
- 已支持显示模型；
- 已支持 ShapeTree 选择与 Viewer 高亮联动；
- 已支持 DiagnosticPanel 选择与 Viewer 高亮联动；
- 已支持显示对象 Bounding Box；
- 已支持基础视角控制；
- 后续补充隐藏 / 显示子 Shape、诊断多选对象包围盒和更深入的拓扑细节联动。

### 验收标准

- 能通过 UI 快速定位异常 Face / Edge / Vertex；
- 诊断结果和三维视图有关联；
- 能辅助判断几何/拓扑问题位置。

### 后续迭代：拓扑细节视窗（Topological Detail Inspector）

**实现状态（阶段 1，已落地）**：右侧 **`TopologyDetailDock`**（`QDockWidget`）随树选中更新——**Face**：`FaceUvCanvasWidget` 在 **(u,v)** 平面绘制各 Wire 的 pcurve 采样折线，外环/内环分色并带走向示意；**Edge**：文本摘要（3D 曲线类型、参数域、容差）+ **`EdgeSchematicWidget`** 用圆盘/点列示意顶点容差与边容差（MVP，非真 3D 球）；**Vertex**：HTML 列出根形体上所有 **邻接 Edge** 及容差。与主 3D 视图的深度联动、真 3D 容差球、SameParameter 图层等仍属 **阶段 2+**。

**目标（完整版）**：在 Dock 或独立窗中展示与拓扑调试强相关的抽象几何，便于对照诊断证据定位 pcurve、容差与邻接问题。

| 子形状 | 规划展示要点（完整版） |
|--------|----------------|
| **Face** | 在曲面 **参数域 (u,v)** 中绘制 Wire 的投影及 **pcurve** 布局，**标明走向**（外环/内环、环方向）；与主 3D 同步高亮、奇异点标注。 |
| **Edge** | **真 3D** 曲线 + 端点；**球体**表示容差；SameParameter、3D 与 pcurve 偏差图层。 |
| **Vertex** | **邻接 Edge/Face 扇区** 星形或局部 3D；交互布局细化。 |

**工程要点**：共享 `ShapeDocument` / 当前 `TopoDS_Shape`；大模型需 **惰性构建 AIS** 或采样；2D 域已用 `QPainter`；后续可加第二 `V3d_View` 或专用 AIS 层。

**阶段 2+ 验收**：与主视图双向联动；Edge/Vertex 具备可旋转的局部 3D 或参数截面；可辅助阅读 BRepCheck 证据。

---

## Milestone 4：问题会话系统（初版进行中）

### 目标

问题可保存、可复现、可沉淀。

### 任务

- 已定义 `.occtdbg` session 格式，见 `doc/session_format.md`；
- 已实现 `SessionSerializer`；
- 已支持保存 / 打开调试会话；
- 已支持记录输入文件、问题描述、参数、诊断结果；
- 已支持导出最小复现目录；
- 后续补充 `operations` 建模、多输入会话和复现包附带 Markdown 报告。

### 验收标准

- 一次调试过程可以保存；
- 下次打开后可以继续分析；
- 可以导出复现包；
- 诊断结果可沉淀为案例。

---

## Milestone 5：EvidenceBundle 可信证据包

### 目标

将一次分析中的所有机器证据统一沉淀为可追溯、可复用、可供 AI 消费的 EvidenceBundle。

### 任务

- 定义 `EvidenceBundle` schema；
- 为每条 evidence 分配稳定 `evidenceId`；
- 将 `ShapeDocument`、BRepCheck、诊断结果、输入文件 hash、环境信息写入证据包；
- 将 Face UV 图、Viewer 截图、Shape 树 JSON 等作为 artifact 引用；
- 扩展 Markdown 报告，使报告引用 EvidenceBundle；
- 在最小复现目录中附带 `evidence.json`。

### 验收标准

- 同一个 `.occtdbg` 可重复生成结构稳定的 `evidence.json`；
- 报告中的每个 Finding 能追溯到 evidenceId；
- EvidenceBundle 不依赖 GUI，可由后续 CLI 生成；
- AI prompt 不直接读取散乱文件，而是读取 EvidenceBundle。

---

## Milestone 6：Headless Analyzer 与调试分析实验室

### 目标

让 OCCTDebug 从 GUI 工具升级为可批量运行的分析器和调试分析实验室，支持问题文档导入、复现沙箱、数据流追踪、CI、批量 case 和自动报告生成。

### 任务

- 增加 headless CLI；
- 支持 `analyze --session debug.occtdbg --out report.md --evidence evidence.json`；
- 支持 `replay --session debug.occtdbg --export-repro repro/`；
- 已支持导入 `problem.md` 生成 `ProblemContext`，并提供 GUI 模板生成入口；
- 支持从问题文档和用户步骤生成初始 `operations`；
- 支持生成 Repro Sandbox；
- 支持 Dataflow Trace Collector，在关键算法节点导出 Shape 快照和参数；
- 支持 Shape Snapshot / Diff；
- 支持 Experiment Ledger，记录假设、命令、观察和结论；
- 支持批量分析 `data/` 或指定目录；
- 支持 STEP assembly batch 检查复用 CLI；
- 输出稳定退出码，便于 CI 使用；
- 支持 JSON/Markdown 双输出。

### 验收标准

- 在无 GUI 环境下能打开 session、运行规则、生成 EvidenceBundle 和 Markdown 报告；
- 用户只提供 `problem.md` 和模型文件时，工具能生成可复现分析目录；
- 数据流追踪结果能进入 EvidenceBundle；
- 批量分析失败 case 时能保留每个 case 的证据目录；
- CI 可以根据严重等级或规则结果判定失败。

---

## Milestone 7：知识库与 AI 可信分析报告

### 目标

从“规则诊断工具”升级为“证据驱动的 AI 分析报告生成器”。

### 任务

- 定义 `knowledge/rules/*.yaml`；
- 定义 `knowledge/cases/*.json`；
- 定义 `knowledge/api/*.yaml`；
- 定义 `knowledge/fixes/*.json`；
- 实现 OCCT Bug Knowledge Collector 的 K0/K1：手动录入、EvidenceBundle 入库、GitHub Issues/Releases 白名单采集；
- 诊断结果关联知识库条目；
- 支持按症状搜索案例；
- 支持将当前会话沉淀为案例；
- 实现 Evidence-grounded Prompt Builder；
- 实现相似案例检索；
- 生成 OCCT Bug Analysis Report；
- 报告中的 AI 结论必须引用 evidenceId。

### 验收标准

- 用户遇到类似问题时，工具可以提示历史案例；
- 诊断结果能链接到规则、案例、API 和修复记录；
- 知识条目包含 source、trustLevel、reviewState；
- AI 报告包含 Summary、Reproduction、Evidence、Root Cause Hypotheses、Recommended Fix、Validation Plan；
- 报告中不存在没有 evidenceId 支撑的根因结论；
- 案例可以不断积累。

---

## Milestone 8：Codex 接管模式

### 目标

让 OCCTDebug 能生成可交给 Codex 执行的调试接管包，使 Codex 可以在受控边界内完成复现、分析、代码修改尝试、测试验证和报告回填。

### 任务

- 定义 `CodexHandoffPackage` schema；
- 从 `problem.md`、`.occtdbg`、EvidenceBundle、知识库和报告生成接管包；
- 生成 `task.md`，描述目标、背景、成功标准；
- 生成 `guardrails.md`，描述允许修改范围、禁止操作、必须运行的测试；
- 生成 `commands/reproduce.*`、`commands/analyze.*`、`commands/build.*`、`commands/test.*`；
- 生成 `experiment_ledger.md`，记录 Codex 的假设、实验和观察；
- 支持 Codex 输出 `codex_debug_report.md`、`patch.diff`、`validation_report.md`；
- 支持将 Codex 输出的 before/after EvidenceBundle 回填到当前 case。

### 验收标准

- 接管包在干净工作区可独立复现问题；
- Codex 能根据接管包执行指定命令，无需猜测项目入口；
- Codex 输出必须包含执行过的命令、关键结果、patch 或未修复原因；
- Codex 未完成验证时，结果不能进入 recommended fix；
- 人工 reviewer 可以根据 `patch.diff`、`validation_report.md` 和 before/after EvidenceBundle 审核结论。

---

## Milestone 9：修复候选与验证流水线

### 目标

在可信证据、历史知识和 Codex 接管结果基础上生成修复候选、回归测试草案和验证报告。

### 任务

- 生成最小复现描述；
- 生成 DRAW script 或 C++ repro 草案；
- 生成 regression test 草案；
- 根据 sourceSuspects 或 Codex 调试结果生成 patch 草案；
- 执行本地构建和目标测试；
- 记录 before/after EvidenceBundle；
- 生成 Fix Validation Report。

### 验收标准

- 修复候选必须附带回归用例或验证命令；
- 修复报告必须包含 before/after 证据差异；
- 未通过验证的 patch 只能作为草案，不得标记为 recommended fix；
- 用户确认后可将结果沉淀到 `knowledge/fixes/`。

---

## Milestone 10：专题诊断插件

### 目标

针对高频 OCCT 问题做专项诊断。插件应输出 EvidenceBundle 扩展字段，而不是只输出 UI 文案。

### 优先顺序

1. Topology 插件；
2. Tolerance 插件；
3. Boolean 插件；
4. Projection 插件；
5. Face Classification 插件；
6. HLR 插件；
7. Meshing 插件；
8. Path Planning 插件。

### 每个插件应包含

- 专题问题输入模板；
- 专题诊断规则；
- 专题可视化；
- 专题 Evidence schema；
- 专题报告章节；
- 专题案例；
- 专题验证工具。

### 验收标准

- 每类常见问题有独立诊断入口；
- 每个专题插件能输出明确的根因候选；
- 每个专题插件至少沉淀 3~5 个真实案例；
- 插件输出能被 AI 报告和修复验证流水线消费。

---

## 16. 当前 MVP 基线与下一版目标

当前 v0.1 基线定义为：

> OCCTDebug v0.1：BREP 结构观察 + 基础诊断 + Markdown 报告。

### 已落地能力

- 打开 `.brep`、`.stp`、`.step` 文件；
- 展示 Shape 拓扑树；
- 显示选中 Shape 属性；
- 执行基础诊断规则；
- 显示诊断结果；
- 点击诊断项定位 Shape；
- 导出 Markdown 诊断报告；
- 保存 / 打开 `.occtdbg` 会话；
- 导出最小复现目录；
- 3D Viewer 基础显示、高亮和 Bounding Box；
- Topology Detail Panel 阶段 1：Face UV/pcurve、Edge/Vertex 摘要。

### 下一版优先目标

- 实现 `Problem Document Importer`，支持 `problem.md` 导入；
- 定义 EvidenceBundle schema；
- 让复现包附带 `evidence.json` 和 Markdown 报告；
- `operations` 序列化，形成可 replay 的操作图；
- 提高诊断结果与 Shape 树节点的映射精度；
- 增加 headless analyze 入口；
- 增加更丰富的单元测试和样例模型。

### 暂不做

- 脱离 EvidenceBundle 的通用 AI 问答；
- 无人审核的自动修复 / 自动合入；
- 未经 EvidenceBundle 支撑的 AI 结论；
- 未经验证的 patch 自动合入；
- STEP 完整 XCAF 语义、颜色、属性和装配树保真导入；
- 完整多版本 OCCT 对比矩阵；
- 复杂可视化动画；
- 插件系统 UI；
- 大规模知识库。

但代码结构需要为这些能力预留接口。

---

## 17. 推荐后续开发顺序

```text
第 1 步：补齐 M4 缺口，实现 operations 序列化，形成可 replay 的问题会话
第 2 步：已实现 Problem Document Importer 和 GUI 模板生成入口；下一步补齐格式校验、输入路径检查和 operations 初始生成
第 3 步：定义 EvidenceBundle schema，并从当前 ShapeDocument / DiagnosticFinding / 导出物生成 evidence.json
第 4 步：改造 MarkdownReportExporter，使报告从 EvidenceBundle 生成并引用 evidenceId
第 5 步：提高 DiagnosticFinding 与 ShapeDocument 节点关联精度，补齐 possibleCauses / relatedShapeIds 输出
第 6 步：增加 headless analyze / replay CLI，支持批量生成 EvidenceBundle 和报告
第 7 步：实现 Repro Sandbox、Command Harness、Dataflow Trace Collector、Shape Snapshot / Diff
第 8 步：建立 knowledge/rules、knowledge/cases、knowledge/api、knowledge/fixes、knowledge/items 的最小结构
第 8.1 步：实现 OCCT Bug Knowledge Collector 的手动录入和 EvidenceBundle 自动入库
第 8.2 步：实现 GitHub Issues / Releases 白名单采集和人工审核流程
第 9 步：实现相似案例检索和 Evidence-grounded Prompt Builder
第 10 步：生成 OCCT Bug Analysis Report，所有 AI 结论必须引用 evidenceId
第 11 步：生成 CodexHandoffPackage，让 Codex 可接管调试、分析、测试
第 12 步：接收 Codex 输出的 patch、测试结果和 before/after EvidenceBundle
第 13 步：生成最小复现、DRAW/C++ repro、regression test 草案
第 14 步：建立 Fix Validation Report，记录 before/after 证据差异
```

---

## 18. 当前类清单与待补齐类

```text
core/
  DebugSession                     已实现
  ProblemContext                    已实现
  DiagnosticFinding                 已实现
  ShapeDocument                     已实现
  ShapeNode                         已实现

occt/
  ShapeInspector                    已实现
  GeometrySummary                   已实现
  FaceUvExtractor                   已实现
  BBoxWire                          已实现
  TopologyInspector                 待按需要拆分
  ToleranceInspector                待按需要拆分
  BRepCheckInspector                待按需要拆分

diagnose/
  IDiagnosticRule                   已实现
  DiagnosticEngine                  已实现
  RuleRegistry                      已实现

diagnose/rules/
  CheckNullShapeRule                已实现
  CheckWireClosedRule               已实现
  CheckShellClosedRule              已实现
  CheckToleranceRule                已实现
  CheckBRepValidityRule             已实现
  CheckSameParameterRule            已实现

io/
  BRepLoader                        已实现，含 BREP/STEP 读入
  ShapeTreeJsonExporter             已实现
  MarkdownReportExporter            已实现
  SessionSerializer                 已实现
  ReproPackageExporter              已实现

ui/
  MainWindow                        已实现
  ShapeTreeWidget                   已实现
  PropertyPanel                     已实现
  DiagnosticPanel                   已实现
  ViewerWidget                      已实现
  TopologyDetailPanel               已实现阶段 1
  FaceUvCanvasWidget                已实现
  EdgeSchematicWidget               已实现

analysis/（规划）
  ProblemDocumentImporter           待实现
  EvidenceBundle                    待实现
  EvidenceBundleWriter              待实现
  EvidenceBundleReader              待实现
  OperationGraphBuilder             待实现
  ReproSandboxGenerator             待实现
  CommandHarness                    待实现
  DataflowTraceCollector            待实现
  ShapeSnapshotDiff                 待实现
  ExperimentLedger                  待实现
  ValidationRunner                  待实现
  SourceContextIndexer              待实现
  CodexHandoffPackage               待实现
  CodexHandoffExporter              待实现
  ResultIngestor                    待实现

cli/（规划）
  occtdebug-cli analyze              待实现
  occtdebug-cli replay               待实现
  occtdebug-cli batch                待实现
```

---

## 19. 文档规划

`doc/` 目录下当前维护和待补齐文档如下：

```text
doc/architecture.md（已存在）
说明整体架构、模块边界、数据流。

doc/session_format.md（已存在）
说明 .occtdbg 调试会话格式。

doc/roadmap.md（已存在）
说明开发里程碑。

doc/diagnostic_rules.md（待创建）
说明规则编号、规则输入、规则输出、严重等级。

doc/plugin_design.md（待创建）
说明插件机制、专题诊断扩展方式。

doc/case_template.md（待创建）
说明如何沉淀一个 OCCT 问题案例。

doc/evidence_bundle.md（待创建）
说明 EvidenceBundle schema、evidenceId、artifact 引用和报告映射。

doc/ai_report_pipeline.md（待创建）
说明 AI 分析报告生成、prompt 约束、验证要求和修复报告流程。

doc/codex_handoff.md（待创建）
说明 Codex 接管包、任务边界、命令约定、输出回填和人工审核流程。

doc/debug_lab.md（待创建）
说明问题文档导入、复现沙箱、数据流追踪、实验记录和验证运行器。

doc/occt_learning_loop.md（待创建）
说明 Codex 的 OCCT 能力分级、知识沉淀、人工反馈和可信度提升机制。

doc/occt_bug_knowledge_collector.md（已存在）
说明 OCCT bug、版本差异、官方 issue、论坛线索、手动录入和 EvidenceBundle 自动入库的采集与审核计划。
```

---

## 20. 开发风险与注意事项

### 20.1 不要让 AI 脱离证据

AI 应该建立在结构化诊断数据和知识库之上。

如果没有 EvidenceBundle、规则诊断、历史案例和验证结果，AI 只能基于文字猜测问题，很难稳定定位根因。AI 可以驱动分析流程，但不能替代证据采集和验证。

### 20.2 不要让 UI 直接绑定 OCCT 逻辑

UI 层只负责展示和交互。

OCCT 解析、诊断规则、数据模型应放在独立模块中。

### 20.3 不要一次性实现所有专题

当前基础闭环已经初步完成：

```text
Shape 加载
拓扑树
属性查看
基础诊断
报告导出
会话保存
最小复现目录导出
基础 3D Viewer
```

后续应在这个闭环稳定后，再逐步扩展 Boolean、Projection、HLR 等专题。

### 20.4 诊断结果必须可验证

每条诊断结果必须有证据。

不要输出无法验证的结论。

### 20.5 保留最小复现能力

很多 OCCT 问题最终都需要构造最小复现。

因此工具需要持续完善：

- 已支持复制当前主输入模型到最小复现目录；
- 已支持导出诊断报告；
- 已支持保存调试会话；
- 后续支持导出相关子 Shape；
- 后续支持导出操作参数；
- 后续支持导出复现代码片段。

---

## 21. 最关键的项目判断

OCCTDebug 的核心不是 Qt 界面，也不是“AI 问答”。

真正核心是五件事：

```text
1. 把 OCCT 对象状态结构化采集出来；
2. 把常见问题转成可执行的诊断规则；
3. 把每次分析编译成可信 EvidenceBundle；
4. 让 AI 基于 EvidenceBundle、知识库和验证结果生成分析报告；
5. 把每次调试和修复沉淀成可复用案例与回归测试。
```

只要这五件事做好，AI 自动化分析和修复辅助会很自然。

否则直接做 AI 诊断，只会变成“根据文字描述猜问题”，无法稳定定位根因，也无法让维护者信任修复结论。

---

## 22. 当前交付闭环

当前已形成的基础闭环：

```text
打开 BREP / STEP
  ↓
构建 ShapeDocument
  ↓
显示 ShapeTree
  ↓
显示 Shape 属性
  ↓
运行基础诊断规则
  ↓
显示 DiagnosticFinding
  ↓
导出 Markdown 报告
  ↓
保存 / 打开 .occtdbg 会话
  ↓
导出最小复现目录
```

下一步目标闭环：

```text
导入 problem.md / 打开或 replay .occtdbg
  ↓
执行结构化采集和规则诊断
  ↓
生成 EvidenceBundle
  ↓
检索知识库和相似案例
  ↓
AI 生成 bug 分析报告
  ↓
生成修复候选和验证计划
  ↓
生成 Codex 接管包
  ↓
Codex 复现 / 访问数据流 / 调试 / 修改 / 测试 / 回填证据
  ↓
运行回归验证
  ↓
生成 Fix Validation Report
  ↓
沉淀案例 / 修复知识 / 回归测试
```

下一步不应再重复建设基础 GUI 闭环，而应优先实现 EvidenceBundle、headless analyze、AI 可信报告、Codex 接管模式和修复验证流水线。
