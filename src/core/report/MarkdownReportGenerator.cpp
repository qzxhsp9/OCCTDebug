#include "core/report/MarkdownReportGenerator.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringConverter>
#include <QTextStream>

namespace occtdebug
{
namespace
{
QString escapedCell(QString value)
{
    value.replace(QLatin1Char('|'), QStringLiteral("\\|"));
    value.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
    return value;
}

void writeLabelValueTable(QTextStream& out, const QVector<LabelValue>& values)
{
    out << "| 字段 | 值 |\n";
    out << "|---|---|\n";
    for (const LabelValue& value : values)
    {
        out << "| " << escapedCell(value.label) << " | " << escapedCell(value.value) << " |\n";
    }
    out << "\n";
}
} // namespace

bool MarkdownReportGenerator::writeReproReport(const CaseManifest& manifest, const QString& filePath, QString* error)
{
    const QFileInfo fileInfo(filePath);
    QDir dir;
    if (!dir.mkpath(fileInfo.absolutePath()))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("failed to create report directory: %1").arg(fileInfo.absolutePath());
        }
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("failed to open report: %1").arg(file.errorString());
        }
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    out << "# " << (manifest.title.isEmpty() ? manifest.caseId : manifest.title) << "\n\n";
    out << "- Case ID: `" << manifest.caseId << "`\n";
    out << "- 状态: " << manifest.status << "\n";
    out << "- 创建时间: " << manifest.createdAt << "\n";
    out << "- OCCT: " << manifest.occtVersion << "\n";
    out << "- 工具链: " << manifest.toolchain << "\n";
    out << "- 平台: " << manifest.platform << "\n\n";

    out << "## 关键输入\n\n";
    writeLabelValueTable(out, manifest.keyInputs);

    out << "## 环境摘要\n\n";
    out << manifest.environmentSummary << "\n\n";

    out << "## 复现脚本\n\n";
    out << "```tcl\n" << manifest.reproScript << "\n```\n\n";

    out << "## 证据链\n\n";
    out << manifest.evidenceSummary << "\n\n";
    out << "| 类型 | 标题 | 摘要 | 链接 |\n";
    out << "|---|---|---|---|\n";
    for (const EvidenceRecord& evidence : manifest.evidenceItems)
    {
        out << "| " << escapedCell(evidence.type)
            << " | " << escapedCell(evidence.title)
            << " | " << escapedCell(evidence.summary)
            << " | `" << escapedCell(evidence.link) << "` |\n";
    }
    out << "\n";

    out << "## 诊断结论\n\n";
    out << manifest.diagnosis << "\n\n";
    out << "置信度: " << manifest.diagnosisConfidence << "%\n\n";

    out << "## 候选补丁\n\n";
    out << "```diff\n" << manifest.patchDiff << "\n```\n\n";

    out << "## 验证结果\n\n";
    writeLabelValueTable(out, manifest.verificationItems);

    out << "## testgrid 摘要\n\n";
    out << "| 模块 | 运行 | 通过 | 失败 | 通过率 |\n";
    out << "|---|---:|---:|---:|---:|\n";
    for (const TestgridRow& row : manifest.testgridRows)
    {
        out << "| " << escapedCell(row.module)
            << " | " << escapedCell(row.runCount)
            << " | " << escapedCell(row.passCount)
            << " | " << escapedCell(row.failCount)
            << " | " << escapedCell(row.passRate) << " |\n";
    }
    out << "\n";

    out << "## 风险与后续\n\n";
    out << "- 当前报告由本地 sample case 生成。\n";
    out << "- 真实复现、真实日志、testgrid/testdiff 接入后应替换 mock 数据。\n";
    out << "- 报告默认不展开敏感绝对路径。\n";

    return true;
}
} // namespace occtdebug
