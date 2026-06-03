# OCCT Bug Knowledge Collector 设计与开发计划

## 1. 价值评估

结论：该模块有明确价值，且应作为 OCCTDebug 的核心长期能力之一，但必须从“可追溯、可验证、可维护”的知识库做起，不能做成无约束的网页内容抓取器。

价值主要体现在：

- **缩短 bug triage 时间**：建模算法问题往往症状相似，例如 Boolean 失败、offset 异常、pcurve 不一致、SameParameter 问题、BRepCheck invalid。历史问题能帮助快速缩小怀疑范围。
- **支撑 AI 可信分析**：AI 自动报告需要可靠上下文。公开 issue、release notes、upgrade notes、论坛讨论、手动案例和本工具生成的 EvidenceBundle 可以成为 RAG 和根因排序的知识源。
- **沉淀版本差异**：OCCT 不同版本在 Boolean、BRepOffset、BRepMesh、STEP、BRepGraph、TopoDS_TShape 等方面存在行为变化。版本差异知识能辅助判断“这是 bug、已修复问题、行为变更还是迁移问题”。
- **服务 Codex 接管**：Codex 调试前可先检索相似历史问题、疑似源码模块、相关测试路径和已知修复模式，减少盲目阅读源码。
- **形成组织记忆**：手动录入的问题、当前工具分析出的结论、修复验证结果都能沉淀为可复用知识。

风险和边界：

- 公开网页内容质量参差不齐，必须保留来源 URL、采集时间、可信等级和人工确认状态。
- 不能默认复制大段网页内容，知识库应保存摘要、结构化字段、链接和少量必要摘录。
- 第三方论坛、博客、FreeCAD 等来源只能作为线索，不能作为最终事实。
- 自动采集必须尊重站点规则、API 限流、授权和用户配置；商业或私有 issue 不应采集。

## 2. 目标定位

模块名称建议为 `OCCT Bug Knowledge Collector`。

目标：

```text
公开来源 / 手动录入 / 工具分析结果
  ↓
采集与去重
  ↓
结构化归类
  ↓
可信度标注
  ↓
知识库入库
  ↓
检索 / RAG / EvidenceBundle / CodexHandoff 使用
```

该模块不直接判断 bug 根因，而是提供可追溯的知识储备：

- bug 症状；
- 影响版本；
- 涉及模块；
- 相关 API；
- 输入模型或复现条件；
- 解决方案或 workaround；
- 修复 commit / PR / release；
- 对应测试或回归用例；
- 与当前 EvidenceBundle 的相似度。

## 3. 数据来源

### 3.1 优先来源

| 来源 | 价值 | 可信等级 | 采集方式 |
|---|---|---|---|
| GitHub Issues: `Open-Cascade-SAS/OCCT` | 当前公开 issue 主入口 | 高 | GitHub API |
| GitHub Releases | 版本修复和行为变化 | 高 | GitHub API / release feed |
| OCCT upgrade notes | 版本迁移和 API 行为变化 | 高 | 文档抓取 / 手动索引 |
| OCCT issue tracker 页面 | 指向 GitHub Issues 和 Mantis archive | 高 | 手动配置入口 |
| Mantis public archive | 历史 bug 线索 | 中高 | 只读抓取 / 手动导入 |
| OCCT forum | 复现经验、官方回复、常见建模问题 | 中 | 搜索 + 摘要 |
| 当前工具 EvidenceBundle / Fix Validation Report | 本项目内部已验证知识 | 最高 | 自动入库 |
| 手动录入 | 用户确认问题和解决方案 | 取决于 reviewer | 表单 / Markdown |

依据当前公开信息，OCCT 官方 GitHub 仓库说明其是 3D surface/solid modeling、CAD/CAM/CAE 的 C++ 平台；官方 issue tracker 页面现在指向 GitHub Issues，并说明旧 Mantis 已退役且提供只读 archive；官方迁移公告也说明公开开源问题应提交到 GitHub Issues，活动开发迁移到 GitHub。GitHub Releases 和 OCCT upgrade notes 可提供版本差异与修复信息。OCCT 自动化测试文档也说明 bug 测试通常放在 `$CASROOT/tests/bugs`，并通过 TODO / REQUIRED 等机制表达已知问题和期望结果。

### 3.2 初期关注范围

优先关注建模算法：

- Boolean：`BRepAlgoAPI_*`、`BOPAlgo_*`；
- Topology：wire、shell、solid、non-manifold；
- Tolerance：vertex / edge / face tolerance、容差放大；
- SameParameter / SameRange；
- pcurve / curve-on-surface；
- Offset / Fillet / Chamfer；
- Projection / Classification；
- BRepCheck invalid；
- Meshing 对建模结果的暴露问题；
- Data Exchange 导入导致的建模异常。

经典问题也可以采集，但需要标注 category，例如 Visualization、Data Exchange、Crash、Performance、Build。

## 4. 知识数据模型

建议核心实体如下：

```text
KnowledgeItem
  id
  type: issue | forum | release_note | upgrade_note | manual | evidence | fix
  source
  url
  collectedAt
  title
  summary
  rawExcerpt
  licenseNote
  trustLevel
  reviewState
  categories[]
  occtVersions[]
  modules[]
  apis[]
  symptoms[]
  rootCause
  workaround
  fix
  relatedIssues[]
  relatedCommits[]
  relatedTests[]
  relatedEvidence[]
  tags[]
```

可信度建议：

| trustLevel | 含义 |
|---|---|
| official_verified | 官方 release / upgrade / merged fix / 已验证内部 EvidenceBundle |
| official_reported | GitHub issue / Mantis archive / 官方论坛回复 |
| community_reported | 社区论坛、FreeCAD 讨论、博客 |
| manual_verified | 用户手动录入并经过本地验证 |
| ai_suggested | AI 从证据中提出，尚未人工确认 |

reviewState：

```text
new
triaged
verified
rejected
obsolete
merged_to_case
```

## 5. 模块架构

```text
OCCT Bug Knowledge Collector
├── Source Connectors
│   ├── GitHubIssueConnector
│   ├── GitHubReleaseConnector
│   ├── OcctDocsConnector
│   ├── OcctForumConnector
│   ├── MantisArchiveConnector
│   ├── ManualEntryConnector
│   └── EvidenceIngestConnector
│
├── Normalization
│   ├── HtmlCleaner
│   ├── MarkdownExtractor
│   ├── MetadataExtractor
│   ├── VersionExtractor
│   ├── ApiMentionExtractor
│   └── ModuleClassifier
│
├── Storage
│   ├── knowledge/items/*.json
│   ├── knowledge/index.sqlite
│   ├── knowledge/raw-cache/
│   └── knowledge/embeddings/
│
├── Review UI
│   ├── 待审核条目
│   ├── 合并重复条目
│   ├── 手动录入
│   ├── 可信度标注
│   └── 关联 EvidenceBundle
│
├── Retrieval
│   ├── Keyword Search
│   ├── API / Module Search
│   ├── Version Difference Search
│   ├── Similar Case Search
│   └── RAG Context Builder
│
└── Export
    ├── EvidenceBundle links
    ├── AI prompt context
    ├── CodexHandoff source_context.json
    └── Bug analysis report sections
```

## 6. 与 OCCTDebug 现有体系的关系

该模块应和已有路线这样连接：

- `EvidenceBundle`：分析结果可自动生成 `KnowledgeItem(type=evidence)`。
- `Fix Validation Report`：通过验证的修复进入 `KnowledgeItem(type=fix)`。
- `CodexHandoffPackage`：生成 `source_context.json` 时检索相似 bug、相关 API、版本差异和历史修复。
- `AI Report Pipeline`：RAG 只使用已审核或明确标注可信等级的知识条目。
- `Problem Document Importer`：用户手写问题可选择同时作为 manual knowledge 草稿入库。

## 7. 合规与质量控制

必须实现：

- 站点白名单；
- 采集频率限制；
- 来源 URL 和采集时间记录；
- raw cache 可关闭；
- 大段内容不进入报告，只保存摘要和链接；
- 用户可删除采集条目；
- 手动审核流程；
- AI 生成摘要必须标记 `ai_suggested`，不能直接标记为 verified；
- 私有文件、商业数据、登录后内容默认不采集。

## 8. 开发计划

### K0：价值验证和手动录入

目标：先证明知识库能辅助分析，不急于大规模自动抓取。

任务：

- 定义 `KnowledgeItem` JSON schema；
- 增加 `knowledge/items/`；
- 支持手动录入 OCCT 问题和解决方案；
- 支持从当前 EvidenceBundle 生成知识草稿；
- 支持本地关键字搜索。

验收：

- 可手动录入 10 条建模算法问题；
- 当前分析出的 Finding 可关联到知识条目；
- Bug 分析报告能引用知识条目链接。

### K1：GitHub Issues / Releases 采集

目标：接入当前最重要的公开官方来源。

任务：

- 实现 GitHub Issues connector；
- 实现 GitHub Releases connector；
- 支持按 label、keyword、module、version 过滤；
- 初期关键词聚焦 Boolean、BRepCheck、offset、fillet、pcurve、SameParameter、tolerance；
- 保存摘要、状态、标签、版本、URL，不保存大段原文。

验收：

- 可增量采集 OCCT GitHub Issues；
- 可生成建模算法相关条目列表；
- 可按 API / 模块 / 版本检索。

### K2：OCCT Docs / Upgrade Notes / Mantis Archive

目标：补充版本差异和历史问题。

任务：

- 采集 upgrade notes 中建模算法相关条目；
- 采集 release notes 中 bug fix 和 breaking change；
- 评估 Mantis archive 可访问性和采集格式；
- 将旧 Mantis ID 与 GitHub issue / release note 关联。

验收：

- 可查询某个 OCCT 版本的建模算法行为变化；
- 可在 bug 分析报告中提示“可能是版本差异 / 已知迁移问题”。

### K3：论坛和社区来源

目标：把论坛经验作为线索，而不是最终事实。

任务：

- 支持 OCCT forum 搜索采集；
- 对 FreeCAD / 社区讨论只做可选 connector；
- 标注 `community_reported`；
- 提供人工审核和去重。

验收：

- 社区来源不会自动进入 verified；
- AI 报告引用社区来源时明确标注“线索，未验证”。

### K4：RAG 和 Codex 接管集成

目标：让知识库真正参与分析。

任务：

- 构建向量索引或混合检索；
- 根据 EvidenceBundle 生成检索 query；
- 为 CodexHandoff 生成 `source_context.json`；
- 为 AI report 提供相似案例、版本差异、相关 API 和历史修复。

验收：

- 对一个 Boolean / tolerance / pcurve case，能检索到相关知识条目；
- Codex 接管包中包含可信知识上下文；
- 报告中每个引用都带 source 和 trustLevel。

## 9. 近期落地建议

第一阶段不要直接做“全网爬虫”。建议先做：

1. 手动录入 + EvidenceBundle 自动入库；
2. GitHub Issues / Releases 白名单采集；
3. upgrade notes 版本差异索引；
4. 本地 review UI；
5. 与 AI Report / CodexHandoff 对接。

这样能快速建立有质量的知识储备，同时避免爬取噪音、版权风险和维护成本失控。
