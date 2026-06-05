#include "core/report/MarkdownReportGenerator.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringConverter>
#include <QTextStream>

namespace occtdebug
{
namespace
{
struct ArtifactLinkStatus
{
    QString displayPath;
    QString markdownTarget;
    QString status;
    bool linkable = false;
};

QString escapedCell(QString value)
{
    value.replace(QLatin1Char('|'), QStringLiteral("\\|"));
    value.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
    return value;
}

QString escapedMarkdownLinkText(QString value)
{
    value.replace(QLatin1Char('\\'), QLatin1Char('/'));
    value.replace(QLatin1Char('['), QStringLiteral("\\["));
    value.replace(QLatin1Char(']'), QStringLiteral("\\]"));
    return value;
}

QString sanitized(QString value)
{
    value.replace(QLatin1Char('\\'), QLatin1Char('/'));
    const QRegularExpression drivePath(QStringLiteral(R"(\b[A-Za-z]:/[^\r\n`|<>"']+)"));
    const QRegularExpression uncPath(QStringLiteral(R"((^|[\s(])//[^ \t\r\n`|<>"']+)"));
    value.replace(drivePath, QStringLiteral("<local-path>"));
    value.replace(uncPath, QStringLiteral("\\1<local-path>"));
    return value;
}

QString normalizedPath(QString value)
{
    value = value.trimmed();
    value.replace(QLatin1Char('\\'), QLatin1Char('/'));
    return QDir::cleanPath(value);
}

QString normalizedAbsolutePath(const QString& value)
{
    return normalizedPath(QFileInfo(value).absoluteFilePath());
}

bool isInsideDirectory(const QString& rootPath, const QString& candidatePath)
{
    QString root = normalizedAbsolutePath(rootPath);
    QString candidate = normalizedAbsolutePath(candidatePath);
#ifdef Q_OS_WIN
    root = root.toLower();
    candidate = candidate.toLower();
#endif
    return candidate == root || candidate.startsWith(root + QLatin1Char('/'));
}

bool isExternalOrAbsoluteLink(const QString& link)
{
    const QString normalized = normalizedPath(link);
    const QRegularExpression scheme(QStringLiteral(R"(^[A-Za-z][A-Za-z0-9+.-]*:)"));
    return QDir::isAbsolutePath(normalized)
        || normalized.startsWith(QStringLiteral("//"))
        || scheme.match(normalized).hasMatch();
}

QDir reportDirectory(const QFileInfo& reportFile)
{
    return QDir(reportFile.absolutePath());
}

QDir caseRootDirectory(const QFileInfo& reportFile)
{
    QDir root(reportFile.absolutePath());
    if (root.dirName().compare(QStringLiteral("report"), Qt::CaseInsensitive) == 0)
    {
        root.cdUp();
    }
    return root;
}

ArtifactLinkStatus resolveArtifactLink(const QString& rawLink, const QDir& caseRoot, const QDir& reportDir)
{
    ArtifactLinkStatus status;
    const QString trimmedLink = rawLink.trimmed();
    if (trimmedLink.isEmpty())
    {
        status.displayPath = QString();
        status.status = QStringLiteral("missing link");
        return status;
    }

    const QString link = normalizedPath(trimmedLink);
    status.displayPath = sanitized(link);
    if (isExternalOrAbsoluteLink(link))
    {
        status.displayPath = QStringLiteral("<local-path>");
        status.status = QStringLiteral("blocked absolute/external path");
        return status;
    }

    const QString absoluteTarget = normalizedPath(caseRoot.absoluteFilePath(link));
    if (!isInsideDirectory(caseRoot.absolutePath(), absoluteTarget))
    {
        status.status = QStringLiteral("blocked outside case");
        return status;
    }
    if (!QFileInfo::exists(absoluteTarget))
    {
        status.status = QStringLiteral("missing artifact");
        return status;
    }

    status.status = QStringLiteral("ok");
    status.linkable = true;
    status.markdownTarget = normalizedPath(reportDir.relativeFilePath(absoluteTarget));
    return status;
}

QString evidenceLinkCell(const EvidenceRecord& evidence, const QDir& caseRoot, const QDir& reportDir)
{
    const ArtifactLinkStatus link = resolveArtifactLink(evidence.link, caseRoot, reportDir);
    if (link.linkable)
    {
        return QStringLiteral("[%1](%2)<br>`%3`")
            .arg(escapedMarkdownLinkText(link.displayPath), link.markdownTarget, link.status);
    }
    return QStringLiteral("`%1`<br>%2").arg(link.displayPath, link.status);
}

void writeLabelValueTable(QTextStream& out, const QVector<LabelValue>& values)
{
    out << "| 字段 | 值 |\n";
    out << "|---|---|\n";
    for (const LabelValue& value : values)
    {
        out << "| " << escapedCell(sanitized(value.label)) << " | " << escapedCell(sanitized(value.value)) << " |\n";
    }
    out << "\n";
}

void writeInputFileTable(QTextStream& out, const QVector<InputFileRecord>& files)
{
    out << "| Path | Original name | Bytes | SHA-256 | Imported at |\n";
    out << "|---|---|---:|---|---|\n";
    if (files.isEmpty())
    {
        out << "| _none_ |  | 0 |  |  |\n";
    }
    for (const InputFileRecord& file : files)
    {
        out << "| " << escapedCell(sanitized(file.path))
            << " | " << escapedCell(sanitized(file.originalName))
            << " | " << file.bytes
            << " | `" << escapedCell(file.sha256) << "`"
            << " | " << escapedCell(file.importedAt) << " |\n";
    }
    out << "\n";
}

void writeArtifactSummary(QTextStream& out, const QVector<EvidenceRecord>& evidenceItems, const QDir& caseRoot, const QDir& reportDir)
{
    out << "## Artifact link check\n\n";
    out << "| Artifact | Status |\n";
    out << "|---|---|\n";
    if (evidenceItems.isEmpty())
    {
        out << "| _none_ | no evidence links |\n";
    }
    for (const EvidenceRecord& evidence : evidenceItems)
    {
        const ArtifactLinkStatus link = resolveArtifactLink(evidence.link, caseRoot, reportDir);
        const QString display = link.linkable
            ? QStringLiteral("[%1](%2)").arg(escapedMarkdownLinkText(link.displayPath), link.markdownTarget)
            : QStringLiteral("`%1`").arg(link.displayPath);
        out << "| " << escapedCell(display) << " | " << escapedCell(link.status) << " |\n";
    }
    out << "\n";
}
} // namespace

bool MarkdownReportGenerator::writeReproReport(const CaseManifest& manifest, const QString& filePath, QString* error)
{
    const QFileInfo fileInfo(filePath);
    const QDir caseRoot = caseRootDirectory(fileInfo);
    const QDir reportDir = reportDirectory(fileInfo);
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

    out << "## 输入文件摘要\n\n";
    writeInputFileTable(out, manifest.inputFiles);

    out << "## 环境摘要\n\n";
    out << sanitized(manifest.environmentSummary) << "\n\n";

    out << "## 复现脚本\n\n";
    out << "```tcl\n" << manifest.reproScript << "\n```\n\n";

    out << "## 证据链\n\n";
    out << sanitized(manifest.evidenceSummary) << "\n\n";
    const QString evidenceBundlePath = caseRoot.absoluteFilePath(QStringLiteral("artifacts/evidence_bundle.json"));
    if (QFileInfo::exists(evidenceBundlePath))
    {
        out << "结构化证据包: [artifacts/evidence_bundle.json]("
            << normalizedPath(reportDir.relativeFilePath(evidenceBundlePath))
            << ")\n\n";
    }
    out << "| 类型 | 标题 | 摘要 | 链接 |\n";
    out << "|---|---|---|---|\n";
    for (const EvidenceRecord& evidence : manifest.evidenceItems)
    {
        out << "| " << escapedCell(evidence.type)
            << " | " << escapedCell(sanitized(evidence.title))
            << " | " << escapedCell(sanitized(evidence.summary))
            << " | " << escapedCell(evidenceLinkCell(evidence, caseRoot, reportDir)) << " |\n";
    }
    out << "\n";
    const QString verificationReportPath = caseRoot.absoluteFilePath(QStringLiteral("report/verification_report.md"));
    if (QFileInfo::exists(verificationReportPath))
    {
        out << "Verification report: [report/verification_report.md]("
            << normalizedPath(reportDir.relativeFilePath(verificationReportPath))
            << ")\n\n";
    }
    writeArtifactSummary(out, manifest.evidenceItems, caseRoot, reportDir);

    out << "## 诊断结论\n\n";
    out << sanitized(manifest.diagnosis) << "\n\n";
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
