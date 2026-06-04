#include "workbench/WorkbenchMockData.h"

#include "core/verify/VerificationResultParser.h"

#include <QDir>
#include <QFile>

namespace occtdebug
{
namespace
{
QString sampleCaseDirectory()
{
#ifdef OCCTDEBUG_SOURCE_DIR
    return QStringLiteral(OCCTDEBUG_SOURCE_DIR "/sample_cases/OCC-LOCAL-2026-0001");
#else
    return QString();
#endif
}

QString sampleCaseFilePath()
{
    const QString directory = sampleCaseDirectory();
    return directory.isEmpty() ? QString() : QDir(directory).filePath(QStringLiteral("case.json"));
}

QString readTextFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return QString();
    }
    return QString::fromUtf8(file.readAll());
}

void loadExternalVerificationFiles(WorkbenchMockData& data)
{
    const QString directory = sampleCaseDirectory();
    if (directory.isEmpty())
    {
        return;
    }

    const QDir verificationDir(QDir(directory).filePath(QStringLiteral("verification")));
    const QString testgridText = readTextFile(verificationDir.filePath(QStringLiteral("testgrid_summary.txt")));
    const QVector<TestgridRow> testgridRows = VerificationResultParser::parseTestgridText(testgridText);
    if (!testgridRows.isEmpty())
    {
        data.testgridRows = testgridRows;
        data.manifest.testgridRows = testgridRows;
    }

    const QString testdiffText = readTextFile(verificationDir.filePath(QStringLiteral("testdiff_summary.txt")));
    const TestdiffSummary testdiff = VerificationResultParser::parseTestdiffText(testdiffText);
    if (!testdiff.entries.isEmpty())
    {
        const QString parsedSummary = testdiff.summaryText();
        data.diffSummary = data.diffSummary.isEmpty()
            ? parsedSummary
            : QStringLiteral("%1\n\n%2").arg(data.diffSummary, parsedSummary);
        data.manifest.diffSummary = data.diffSummary;
    }
}
} // namespace

WorkbenchMockData createWorkbenchDataFromCase(const CaseManifest& manifest)
{
    WorkbenchMockData data;
    data.manifest = manifest;
    data.caseId = manifest.caseId;
    data.caseStatus = manifest.status;
    data.occtVersion = manifest.occtVersion;
    data.toolchain = manifest.toolchain;
    data.platform = manifest.platform;
    data.sourceText = manifest.sourceText;
    data.reproScript = manifest.reproScript;
    data.geometrySummary = manifest.geometrySummary;
    data.evidenceSummary = manifest.evidenceSummary;
    data.diffSummary = manifest.diffSummary;
    data.environmentSummary = manifest.environmentSummary;
    data.diagnosis = manifest.diagnosis;
    data.diagnosisConfidence = manifest.diagnosisConfidence;
    data.patchDiff = manifest.patchDiff;
    data.drawConsoleText = manifest.drawConsoleText;
    data.cmakeConsoleText = manifest.cmakeConsoleText;
    data.cases = manifest.caseList;
    data.workflowSteps = manifest.workflowSteps;
    data.keyInputs = manifest.keyInputs;
    data.geometryChecks = manifest.geometryChecks;
    data.evidenceItems = manifest.evidenceItems;
    data.verificationItems = manifest.verificationItems;
    data.similarCases = manifest.similarCases;
    data.testgridRows = manifest.testgridRows;

    if (data.cases.isEmpty())
    {
        data.cases.push_back({manifest.caseId, manifest.status, manifest.title, manifest.createdAt});
    }

    return data;
}

WorkbenchMockData createMockWorkbenchData()
{
    QString loadError;
    const QString filePath = sampleCaseFilePath();
    if (!filePath.isEmpty())
    {
        const std::optional<CaseManifest> manifest = CaseManifest::loadFromFile(filePath, &loadError);
        if (manifest.has_value())
        {
            WorkbenchMockData data = createWorkbenchDataFromCase(*manifest);
            loadExternalVerificationFiles(data);
            return data;
        }
    }

    WorkbenchMockData data;
    data.manifest.caseId = QStringLiteral("OCC-LOCAL-2026-0001");
    data.manifest.title = QString::fromUtf8("Fillet 更新失败，Null curve 导致崩溃");
    data.manifest.status = QString::fromUtf8("已复现 / 分析中");
    data.manifest.createdAt = QStringLiteral("2026-05-19 14:32");
    data.caseId = QStringLiteral("OCC-LOCAL-2026-0001");
    data.caseStatus = QString::fromUtf8("已复现 / 分析中");
    data.occtVersion = QStringLiteral("OCCT 7.8.1");
    data.toolchain = QStringLiteral("VS2026 x64");
    data.platform = QStringLiteral("Windows x64");

    data.cases = {
        {QStringLiteral("OCC-LOCAL-2026-0001"), QString::fromUtf8("分析中"), QString::fromUtf8("Fillet 更新失败，Null curve 导致崩溃"), QStringLiteral("2026-05-19 14:32")},
        {QStringLiteral("OCC-LOCAL-2026-0002"), QString::fromUtf8("待复现"), QString::fromUtf8("Sweep 退化面导致异常"), QStringLiteral("2026-05-18 10:21")},
        {QStringLiteral("OCC-LOCAL-2026-0003"), QString::fromUtf8("已归档"), QString::fromUtf8("Boolean 切割后形状不闭合"), QStringLiteral("2026-05-16 16:08")},
        {QStringLiteral("OCC-LOCAL-2026-0004"), QString::fromUtf8("已归档"), QString::fromUtf8("STEP 导入 NURBS 面丢失"), QStringLiteral("2026-05-15 09:43")},
    };

    data.workflowSteps = {
        {QString::fromUtf8("●"), QString::fromUtf8("1 问题描述"), QString::fromUtf8("已完成")},
        {QString::fromUtf8("●"), QString::fromUtf8("2 环境采集"), QString::fromUtf8("已完成")},
        {QString::fromUtf8("●"), QString::fromUtf8("3 自动复现"), QString::fromUtf8("已完成")},
        {QString::fromUtf8("●"), QString::fromUtf8("4 最小化数据"), QString::fromUtf8("已完成")},
        {QString::fromUtf8("●"), QString::fromUtf8("5 证据收集"), QString::fromUtf8("已完成")},
        {QString::fromUtf8("▶"), QString::fromUtf8("6 根因分析"), QString::fromUtf8("进行中")},
        {QString::fromUtf8("○"), QString::fromUtf8("7 补丁生成"), QString::fromUtf8("未开始")},
        {QString::fromUtf8("○"), QString::fromUtf8("8 回归验证"), QString::fromUtf8("未开始")},
        {QString::fromUtf8("○"), QString::fromUtf8("9 归档沉淀"), QString::fromUtf8("未开始")},
    };

    data.keyInputs = {
        {QString::fromUtf8("输入模型"), QStringLiteral("valve_body_min.brep")},
        {QString::fromUtf8("复现类型"), QString::fromUtf8("DRAW 脚本复现")},
        {QString::fromUtf8("失败模式"), QString::fromUtf8("崩溃 / Null curve")},
        {QString::fromUtf8("保密级别"), QString::fromUtf8("内部")},
    };

    data.sourceText = QString::fromUtf8(
        "src/BRepFilletAPI/BRepFilletAPI_MakeFillet.cxx\n\n"
        "2263\n"
        "2264     if (aNewEdge.IsNull())\n"
        "2265     {\n"
        "2266         Standard_ConstructionError::Raise(\"BRepFilletAPI_MakeFillet: Null new edge\");\n"
        "2267     }\n"
        "2268\n"
        "2269     // 更新生成的边\n"
        "2270     Handle(Geom_Curve) aC = BRep_Tool::Curve(aNewEdge, aFirst, aLast);\n"
        "2271  -> if (aC.IsNull())  // <- 可疑：Null curve\n"
        "2272     {\n"
        "2273         SetError(TopAbs_EDGE, aNewEdge, Fillet_NullCurve);\n"
        "2274         return;\n"
        "2275     }\n"
        "2276\n"
        "2277     myGenerated.Append(aNewEdge);\n"
        "2278     UpdateLimits(aNewEdge, aFirst, aLast);");

    data.reproScript = QString::fromUtf8(
        "restore valve_body_min.brep a\n"
        "vdisplay -dispMode 1 a\n"
        "fillet a 10.0\n"
        "# 复现崩溃于 Fillet 更新阶段\n"
        "fit\n"
        "vfit\n"
        "dump a -ov2\n"
        "quit");

    data.geometrySummary = QString::fromUtf8(
        "几何视图 (3D)\n\n"
        "当前样例：阀体模型 / 异常边 E125\n\n"
        "这里将接入 OCCT V3d_View，支持异常 edge/face 高亮、包围盒、选择同步和截图证据。");
    data.geometryChecks = {
        {QStringLiteral("Null Curve"), QString::fromUtf8("异常"), QStringLiteral("1")},
        {QStringLiteral("Invalid P-Curve"), QString::fromUtf8("异常"), QStringLiteral("1")},
        {QStringLiteral("Small Radius"), QString::fromUtf8("警告"), QStringLiteral("<1e-6")},
        {QStringLiteral("Self-Intersection"), QString::fromUtf8("通过"), QString()},
        {QStringLiteral("Non Manifold"), QString::fromUtf8("通过"), QString()},
    };

    data.evidenceSummary = QString::fromUtf8(
        "证据摘要\n"
        "• 异常边 E125 的曲线为空\n"
        "• 对应 p-curve 在参数域上越界\n"
        "• 与相似案例 OCC-2024-1765 一致\n"
        "• 崩溃发生在 Fillet 更新阶段");
    data.evidenceItems = {
        {QStringLiteral("DRAW"), QString::fromUtf8("运行日志"), QString::fromUtf8("Null curve 异常稳定出现"), QStringLiteral("logs/draw.log")},
        {QStringLiteral("Shape"), QString::fromUtf8("几何检查"), QString::fromUtf8("异常边 E125 缺少 3D curve"), QStringLiteral("evidence/shape.json")},
        {QStringLiteral("Source"), QString::fromUtf8("源码定位"), QString::fromUtf8("BRep_Tool::Curve 返回空值"), QStringLiteral("BRepFilletAPI_MakeFillet.cxx:2271")},
    };
    data.diffSummary = QString::fromUtf8("补丁前后几何、图像、性能与 testdiff 结果将在这里对比。");
    data.environmentSummary = QString::fromUtf8("Windows、MSVC、CMake、Qt、OCCT、DRAWEXE、CASROOT 和第三方依赖快照。");

    data.diagnosis = QString::fromUtf8("根因：圆角过程中新生成的边 E125 曲线为空，导致 BRep_Tool::Curve 返回空，后续计算异常。");
    data.diagnosisConfidence = 86;
    data.patchDiff = QString::fromUtf8(
        "BRepFilletAPI_MakeFillet.cxx\n"
        "@@ -2268,7 +2268,11 @@\n"
        "  Handle(Geom_Curve) aC = BRep_Tool::Curve(aNewEdge, aFirst, aLast);\n"
        "- if (aC.IsNull())\n"
        "+ if (aC.IsNull())\n"
        "  {\n"
        "+   // 候选补丁：记录失败边，保留可诊断状态。\n"
        "+   TryRecoverCurve(aNewEdge, aC);\n"
        "+   if (aC.IsNull())\n"
        "    SetError(TopAbs_EDGE, aNewEdge, Fillet_NullCurve);\n"
        "    return;\n"
        "  }");

    data.verificationItems = {
        {QString::fromUtf8("原始问题"), QString::fromUtf8("已复现 / 已修复")},
        {QString::fromUtf8("相关测试"), QStringLiteral("33 / 35")},
        {QStringLiteral("testgrid"), QStringLiteral("293 / 298")},
        {QString::fromUtf8("性能变化"), QStringLiteral("+0.8%")},
        {QString::fromUtf8("回归风险"), QString::fromUtf8("低")},
    };

    data.similarCases = {
        {QStringLiteral("OCC-2024-1765"), QStringLiteral("Fillet Null curve crash"), QStringLiteral("0.92")},
        {QStringLiteral("OCC-2023-1421"), QStringLiteral("Small radius fillet issue"), QStringLiteral("0.78")},
        {QStringLiteral("OCC-2022-0987"), QStringLiteral("Invalid p-curve on edge"), QStringLiteral("0.63")},
    };

    data.drawConsoleText = QString::fromUtf8(
        "DRAW 7.8.1 (64-bit)\n"
        "> restore valve_body_min.brep a\n"
        "-- Reading file ... Done\n"
        "> fillet a 10.0\n"
        "-- Building fillet ...\n"
        "*** Exception Standard_ConstructionError:\n"
        "BRep_Tool::Curve() - Null curve\n>");
    data.cmakeConsoleText = QString::fromUtf8(
        "CMake configure: OK\n"
        "Build type: Debug\n"
        "MSVC 19.44\n"
        "[1/42] Building CXX object ...\n"
        "[42/42] Linking ...\n"
        "Build completed successfully.");
    data.testgridRows = {
        {QStringLiteral("Modeling"), QStringLiteral("98"), QStringLiteral("96"), QStringLiteral("1"), QStringLiteral("97.9%")},
        {QStringLiteral("Visualization"), QStringLiteral("76"), QStringLiteral("75"), QStringLiteral("0"), QStringLiteral("98.7%")},
        {QStringLiteral("Data Exchange"), QStringLiteral("62"), QStringLiteral("61"), QStringLiteral("1"), QStringLiteral("98.4%")},
        {QStringLiteral("Foundation"), QStringLiteral("62"), QStringLiteral("62"), QStringLiteral("0"), QStringLiteral("100.0%")},
        {QString::fromUtf8("总计"), QStringLiteral("298"), QStringLiteral("293"), QStringLiteral("2"), QStringLiteral("98.3%")},
    };

    loadExternalVerificationFiles(data);
    return data;
}
} // namespace occtdebug
