# OCCTDebug 开发计划

## 1. 项目定位

OCCTDebug 是一个用于调试、分析和诊断 Open CASCADE Technology（OCCT）源码与几何算法问题的辅助工具。

项目最终目标是：

> 输入一个 OCCT 问题环境，包括模型、OCCT 版本、调用链、算法参数、日志、异常、几何/拓扑对象状态，工具能够自动采集关键上下文、执行诊断规则、给出可能根因、建议验证步骤，并沉淀为可复用案例。

项目不是一开始就追求“AI 自动诊断所有问题”，而是先建立稳定的工程基础：

```text
问题环境
  ↓
结构化采集
  ↓
可视化观察
  ↓
规则诊断
  ↓
相似案例匹配
  ↓
根因候选 + 验证建议
  ↓
案例沉淀
```

长期目标可以接入 LLM / RAG，但核心基础必须是：

1. 结构化采集 OCCT 对象状态；
2. 建立可执行诊断规则；
3. 沉淀可复现调试案例；
4. 输出带证据链的诊断结果。

---

## 2. 核心目标

### 2.1 短期目标

构建一个可用的 OCCT 模型观察与基础诊断工具。

短期能力包括：

- 加载 BREP 模型；
- 展示 Shape 拓扑树；
- 查看 Shape 类型、容差、包围盒、几何类型；
- 执行基础 BRepCheck；
- 检查 Wire / Shell / Edge / Face 的常见问题；
- 输出诊断结果；
- 导出 Markdown 诊断报告。

### 2.2 中期目标

构建专题诊断能力。

重点支持：

- Topology 诊断；
- Tolerance 诊断；
- Boolean 诊断；
- Projection 诊断；
- Face Classification 诊断；
- HLR 诊断；
- Meshing 诊断；
- Path Planning 诊断。

### 2.3 长期目标

构建 OCCT 问题诊断助手。

长期能力包括：

- 输入问题描述和问题环境；
- 自动选择诊断规则；
- 根据症状匹配历史案例；
- 输出根因候选；
- 给出验证步骤；
- 支持案例沉淀；
- 支持知识库演进；
- 支持智能诊断解释。

---

## 3. 总体架构

建议将 OCCTDebug 分成以下几个层次：

```text
OCCTDebug
├── App/UI 层
│   ├── 主窗口
│   ├── 问题工作区
│   ├── Shape 浏览器
│   ├── 属性面板
│   ├── 诊断结果面板
│   ├── 日志/调用链面板
│   └── 可视化视图
│
├── Debug Session 层
│   ├── 问题环境描述
│   ├── 输入文件管理
│   ├── OCCT 版本信息
│   ├── 算法参数
│   ├── 诊断过程记录
│   └── 可复现工程导出
│
├── Data Capture 层
│   ├── Shape 信息采集
│   ├── Topology 信息采集
│   ├── Geometry 信息采集
│   ├── Tolerance 信息采集
│   ├── Bounding 信息采集
│   ├── Boolean 专题采集
│   ├── Projection 专题采集
│   ├── HLR 专题采集
│   └── 日志与异常采集
│
├── Diagnostic Engine 层
│   ├── 诊断规则
│   ├── 检查项调度
│   ├── 结果评分
│   ├── 根因候选排序
│   └── 修复/验证建议
│
├── Knowledge Base 层
│   ├── 规则库
│   ├── 问题案例库
│   ├── OCCT API 行为说明
│   ├── 常见错误模式
│   └── 版本差异记录
│
└── Plugin 层
    ├── Boolean 诊断插件
    ├── Projection 诊断插件
    ├── Topology 诊断插件
    ├── Tolerance 诊断插件
    ├── Meshing 诊断插件
    ├── HLR 诊断插件
    └── Path Planning 诊断插件
```

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

建议设计 `.occtdbg` 会话文件。

示例：

```json
{
  "version": 1,
  "occtVersion": "7.9.3",
  "createdAt": "2026-05-06",
  "problem": {
    "title": "Boolean cut shell by half-space missing section face",
    "category": "Boolean",
    "description": "..."
  },
  "inputs": [
    {
      "path": "case/input.brep",
      "type": "brep",
      "role": "targetShape"
    }
  ],
  "operations": [
    {
      "type": "BooleanCut",
      "params": {
        "tool": "halfspace",
        "fuzzyValue": 0.0
      }
    }
  ],
  "diagnostics": []
}
```

支持会话后，工具才能实现：

- 重新打开问题；
- 复跑诊断；
- 导出最小复现；
- 积累案例；
- 对比不同 OCCT 版本结果。

---

## 5. 建议目录结构

当前仓库已有：

```text
OCCTDebug/
├── CMakeLists.txt
├── cmake/
├── depends/
├── doc/
└── src/
```

建议逐步扩展为：

```text
OCCTDebug/
├── CMakeLists.txt
├── cmake/
├── depends/
├── doc/
│   ├── architecture.md
│   ├── diagnostic_rules.md
│   ├── session_format.md
│   ├── plugin_design.md
│   ├── roadmap.md
│   └── case_template.md
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
│   │   ├── DebugSession.cpp
│   │   ├── ProblemContext.h
│   │   ├── DiagnosticFinding.h
│   │   ├── ShapeDocument.h
│   │   ├── ShapeDocument.cpp
│   │   └── ShapeNode.h
│   │
│   ├── occt/
│   │   ├── ShapeInspector.h
│   │   ├── ShapeInspector.cpp
│   │   ├── TopologyInspector.h
│   │   ├── TopologyInspector.cpp
│   │   ├── GeometryInspector.h
│   │   ├── GeometryInspector.cpp
│   │   ├── ToleranceInspector.h
│   │   ├── ToleranceInspector.cpp
│   │   ├── BRepCheckInspector.h
│   │   ├── BRepCheckInspector.cpp
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
│   │   ├── StepLoader.h
│   │   ├── StepLoader.cpp
│   │   ├── SessionSerializer.h
│   │   ├── SessionSerializer.cpp
│   │   ├── MarkdownReportExporter.h
│   │   └── MarkdownReportExporter.cpp
│   │
│   ├── ui/
│   │   ├── ShapeTreeWidget.h
│   │   ├── ShapeTreeWidget.cpp
│   │   ├── PropertyPanel.h
│   │   ├── PropertyPanel.cpp
│   │   ├── DiagnosticPanel.h
│   │   ├── DiagnosticPanel.cpp
│   │   ├── SessionPanel.h
│   │   ├── SessionPanel.cpp
│   │   ├── ViewerWidget.h
│   │   └── ViewerWidget.cpp
│   │
│   └── plugins/
│       ├── boolean/
│       ├── projection/
│       ├── topology/
│       ├── tolerance/
│       ├── hlr/
│       └── meshing/
│
├── tests/
│   ├── test_shape_inspector.cpp
│   ├── test_diagnostic_rules.cpp
│   └── cases/
│
└── examples/
    ├── boolean_halfspace_case/
    ├── face_classification_case/
    └── tolerance_case/
```

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

    std::string occtVersion;
    std::string compiler;
    std::string buildType;

    std::vector<std::string> inputFiles;
    std::map<std::string, std::string> parameters;
};
```

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
按严重等级、置信度、关联 Shape 排序
  ↓
输出到 DiagnosticPanel / ReportExporter
```

---

## 8. 第一批诊断规则

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

建议设计一个问题输入向导。

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
```

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

建议主界面分为四个区域：

```text
+------------------------------------------------------+
| 菜单栏：打开模型 / 保存会话 / 执行诊断 / 导出报告      |
+----------------------+-------------------------------+
| Shape Tree           | 3D Viewer                     |
| - Compound           |                               |
|   - Solid            |                               |
|     - Shell          |                               |
|       - Face         |                               |
|       - Edge         |                               |
+----------------------+-------------------------------+
| Property Panel       | Diagnostic Panel              |
| Type: Face           | [Warning] Wire not closed     |
| Surface: Plane       | [Error] Edge tolerance large  |
| Tolerance: 1e-7      |                               |
+----------------------+-------------------------------+
```

### 10.2 初期 UI 功能

- 打开 BREP；
- Shape 树展示；
- 属性面板展示；
- 诊断结果列表；
- 点击诊断结果定位 Shape；
- 导出报告。

### 10.3 中期 UI 功能

- 3D Viewer；
- 选择 Shape 高亮；
- 显示 Bounding Box；
- 显示异常 Shape；
- 显示 Wire 方向；
- 显示 Face 外环/内环；
- 显示 Edge 参数方向；
- 显示 Boolean 前后对比；
- 显示 Section 线。

---

## 11. 可视化能力规划

### 11.1 初期

```text
显示整个 Shape
选择 ShapeTree 节点后高亮对应 Shape
显示 Bounding Box
显示 Face / Edge / Vertex 基本属性
```

### 11.2 中期

```text
显示异常 Shape
显示 tolerance 热力图
显示 Wire 方向
显示 Face 外环 / 内环
显示 Edge 参数方向
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

## 12. 报告系统设计

诊断结果应支持导出 Markdown 报告。

报告模板：

```markdown
# OCCTDebug Diagnostic Report

## 1. Environment

- OCCT Version:
- Build Type:
- Compiler:
- Platform:

## 2. Problem

- Category:
- Description:
- Expected:
- Actual:

## 3. Input Shapes

| ID | Type | Children | Tolerance | BBox | Valid |
|---|---|---|---|---|---|

## 4. Diagnostic Findings

### R301 Edge tolerance too large

Severity: Warning

Evidence:
- Edge #35 tolerance = 0.284
- Edge length = 0.31

Possible Causes:
- Imported dirty geometry
- Boolean tolerance expansion

Suggestions:
- Run ShapeFix_Edge
- Check SameParameter
- Export Edge #35 as minimal case

## 5. Suggested Next Steps

...
```

报告用途：

- 调试记录；
- Issue 描述；
- 知识库案例；
- LLM 输入上下文；
- 团队内部问题复盘。

---

## 13. 知识库设计

知识库是项目后期能否“诊断根因”的关键。

建议分为三类：

```text
knowledge/
├── rules/
├── cases/
└── api/
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

历史案例使用 JSON 维护。

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
  "relatedRules": ["R504"]
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

---

## 14. AI / LLM 诊断接入规划

不要在项目初期把 LLM 放进核心逻辑。

推荐架构：

```text
Diagnostic Engine 输出结构化 findings
        ↓
Knowledge Retrieval 检索规则/案例/API 说明
        ↓
LLM Reasoner 生成自然语言诊断
        ↓
用户确认结果是否正确
        ↓
沉淀新案例
```

LLM 适合做：

- 组织解释；
- 匹配相似案例；
- 生成调试步骤；
- 总结报告；
- 根据上下文排序根因候选。

LLM 不应直接做：

- 直接判断 TopoDS_Shape；
- 直接替代 OCCT 检查；
- 直接猜测没有证据的根因；
- 直接生成不可验证结论。

---

## 15. 里程碑规划

## Milestone 0：工程骨架稳定

### 目标

整理当前工程结构，为后续开发打基础。

### 任务

- 整理 `src/` 目录结构；
- 将 `MainWindow` 只作为 UI 入口；
- 增加 `core/`、`occt/`、`diagnose/`、`io/`、`ui/` 子模块；
- 增加基础日志系统；
- 增加单元测试框架；
- 增加 `doc/architecture.md`；
- 增加 `doc/roadmap.md`。

### 验收标准

- 项目可在 Windows + Visual Studio + Qt6 + OCCT 下稳定构建；
- 启动后能显示 OCCT 版本；
- 代码目录结构清晰；
- 后续模块扩展不需要大规模重构。

---

## Milestone 1：Shape 加载与拓扑树

### 目标

打开 BREP 文件并查看拓扑结构。

### 任务

- 实现 `BRepLoader`；
- 实现 `ShapeDocument`；
- 实现 `ShapeInspector`；
- 实现 `ShapeTreeWidget`；
- 点击树节点显示 Shape 基础属性；
- 支持导出 Shape 树为 JSON。

### 验收标准

- 能打开 `.brep`；
- 能显示 Compound / Solid / Shell / Face / Wire / Edge / Vertex 层级；
- 能查看每个 Shape 的类型、容差、包围盒；
- 能导出基础 Shape 信息。

---

## Milestone 2：基础诊断规则

### 目标

形成第一个“诊断闭环”。

### 任务

- 实现 `IDiagnosticRule`；
- 实现 `DiagnosticEngine`；
- 实现 `RuleRegistry`；
- 实现 5~10 个基础规则；
- UI 显示诊断结果；
- 点击诊断结果能定位到相关 Shape；
- 导出 Markdown 报告。

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

## Milestone 3：可视化 Viewer

### 目标

真正辅助调试几何。

### 任务

- 集成 OCCT `AIS_InteractiveContext`；
- 显示模型；
- ShapeTree 选择与 Viewer 高亮联动；
- DiagnosticPanel 选择与 Viewer 高亮联动；
- 显示异常对象 Bounding Box；
- 支持隐藏 / 显示子 Shape；
- 支持基础视角控制。

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

## Milestone 4：问题会话系统

### 目标

问题可保存、可复现、可沉淀。

### 任务

- 定义 `.occtdbg` session 格式；
- 实现 `SessionSerializer`；
- 支持保存 / 打开调试会话；
- 支持记录输入文件、问题描述、参数、诊断结果；
- 支持导出最小复现目录。

### 验收标准

- 一次调试过程可以保存；
- 下次打开后可以继续分析；
- 可以导出复现包；
- 诊断结果可沉淀为案例。

---

## Milestone 5：专题诊断插件

### 目标

针对高频 OCCT 问题做专项诊断。

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
- 专题报告章节；
- 专题案例；
- 专题验证工具。

### 验收标准

- 每类常见问题有独立诊断入口；
- 每个专题插件能输出明确的根因候选；
- 每个专题插件至少沉淀 3~5 个真实案例。

---

## Milestone 6：案例库与知识库

### 目标

从“调试工具”升级为“诊断助手”。

### 任务

- 定义 `knowledge/rules/*.yaml`；
- 定义 `knowledge/cases/*.json`；
- 定义 `knowledge/api/*.yaml`；
- 诊断结果关联知识库条目；
- 支持按症状搜索案例；
- 支持将当前会话沉淀为案例。

### 验收标准

- 用户遇到类似问题时，工具可以提示历史案例；
- 诊断结果能链接到规则和案例；
- 案例可以不断积累。

---

## Milestone 7：智能诊断助手

### 目标

输入问题描述后，自动组织诊断流程。

### 任务

- 将 `ProblemContext + DiagnosticFinding` 转为结构化 prompt；
- 检索相关规则和案例；
- 生成根因候选排序；
- 生成下一步调试建议；
- 允许用户反馈“正确 / 不正确”；
- 根据反馈更新案例库。

### 验收标准

用户输入：

```text
shell 用 half-space cut 后缺少截面 face
```

工具输出：

```text
识别问题类型：
- Boolean
- Sheet Body
- HalfSpace Cut

可能根因：
- 当前输入是 Shell，不是 Solid；
- OCCT Boolean 对开放 Sheet Body 做 CUT 时，不会自动生成封闭截面 Face；

建议：
- 如果目标是 sheet 裁剪，使用 Section + Split；
- 如果目标是 solid cut，先将 shell 构造成封闭 solid；
- 如果需要截面 Face，需要显式构建 section wire 并补面。
```

---

## 16. 推荐第一版 MVP

第一版建议定义为：

> OCCTDebug v0.1：BREP 结构观察 + 基础诊断 + Markdown 报告。

### 功能清单

- 打开 `.brep` 文件；
- 展示 Shape 拓扑树；
- 显示选中 Shape 属性；
- 执行基础诊断规则；
- 显示诊断结果；
- 点击诊断项定位 Shape；
- 导出 Markdown 诊断报告。

### 暂不做

- 复杂 AI；
- 自动修复；
- STEP 完整导入；
- 多版本 OCCT 对比；
- 复杂可视化动画；
- 插件系统 UI；
- 大规模知识库。

但代码结构需要为这些能力预留接口。

---

## 17. 推荐开发顺序

```text
第 1 步：整理目录结构
第 2 步：定义 DebugSession / ShapeDocument / DiagnosticFinding
第 3 步：实现 BRepLoader
第 4 步：实现 ShapeInspector，遍历 TopoDS_Shape
第 5 步：实现 ShapeTreeWidget
第 6 步：实现 PropertyPanel
第 7 步：实现 IDiagnosticRule / DiagnosticEngine
第 8 步：实现 CheckToleranceRule / CheckBRepValidityRule
第 9 步：实现 DiagnosticPanel
第 10 步：实现 MarkdownReportExporter
第 11 步：增加 samples/cases 测试模型目录
第 12 步：把历史 OCCT 问题整理为第一批诊断案例
```

---

## 18. 第一批建议实现的类

```text
core/
  DebugSession
  ProblemContext
  DiagnosticFinding
  ShapeDocument
  ShapeNode

occt/
  BRepLoader
  ShapeInspector
  TopologyInspector
  ToleranceInspector
  BRepCheckInspector

diagnose/
  IDiagnosticRule
  DiagnosticEngine
  RuleRegistry

diagnose/rules/
  CheckNullShapeRule
  CheckWireClosedRule
  CheckShellClosedRule
  CheckToleranceRule
  CheckBRepValidityRule
  CheckSameParameterRule

io/
  JsonExporter
  MarkdownReportExporter
  SessionSerializer

ui/
  MainWindow
  ShapeTreeWidget
  PropertyPanel
  DiagnosticPanel
  ViewerWidget
```

---

## 19. 初始文档规划

`doc/` 目录下建议先维护以下文档：

```text
doc/architecture.md
说明整体架构、模块边界、数据流。

doc/session_format.md
说明 .occtdbg 调试会话格式。

doc/diagnostic_rules.md
说明规则编号、规则输入、规则输出、严重等级。

doc/plugin_design.md
说明插件机制、专题诊断扩展方式。

doc/roadmap.md
说明开发里程碑。

doc/case_template.md
说明如何沉淀一个 OCCT 问题案例。
```

---

## 20. 开发风险与注意事项

### 20.1 不要过早引入 AI

AI 应该建立在结构化诊断数据和知识库之上。

如果没有数据采集和规则诊断，AI 只能基于文字猜测问题，很难稳定定位根因。

### 20.2 不要让 UI 直接绑定 OCCT 逻辑

UI 层只负责展示和交互。

OCCT 解析、诊断规则、数据模型应放在独立模块中。

### 20.3 不要一次性实现所有专题

建议先完成：

```text
Shape 加载
拓扑树
属性查看
基础诊断
报告导出
```

再逐步扩展 Boolean、Projection、HLR 等专题。

### 20.4 诊断结果必须可验证

每条诊断结果必须有证据。

不要输出无法验证的结论。

### 20.5 保留最小复现能力

很多 OCCT 问题最终都需要构造最小复现。

因此工具后期必须支持：

- 导出相关 Shape；
- 导出诊断报告；
- 导出操作参数；
- 导出复现代码片段；
- 保存调试会话。

---

## 21. 最关键的项目判断

OCCTDebug 的核心不是 Qt 界面，也不是 AI。

真正核心是三件事：

```text
1. 把 OCCT 对象状态结构化采集出来；
2. 把常见问题转成可执行的诊断规则；
3. 把每次调试过程沉淀成可复用案例。
```

只要这三件事做好，后续接入智能诊断会很自然。

否则直接做 AI 诊断，只会变成“根据文字描述猜问题”，无法稳定定位根因。

---

## 22. v0.1 推荐交付目标

v0.1 建议只追求一个完整闭环：

```text
打开 BREP
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
```

v0.1 完成后，项目就具备继续迭代的基础。
