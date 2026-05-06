#include "io/MarkdownReportExporter.h"

#include "occt/OcctUtils.h"

#include <QSysInfo>

namespace
{
const char* severityString(DiagnosticSeverity s)
{
    switch (s)
    {
    case DiagnosticSeverity::Critical:
        return "Critical";
    case DiagnosticSeverity::Error:
        return "Error";
    case DiagnosticSeverity::Warning:
        return "Warning";
    case DiagnosticSeverity::Info:
    default:
        return "Info";
    }
}
} // namespace

QString MarkdownReportExporter::exportReport(
    const ProblemContext& context,
    const ShapeDocument& document,
    const std::vector<DiagnosticFinding>& findings)
{
    QString md;
    md += QStringLiteral("# OCCTDebug Diagnostic Report\n\n");
    md += QStringLiteral("## 1. Environment\n\n");
    md += QStringLiteral("- OCCT Version: %1\n").arg(QString::fromStdString(context.occtVersion));
    md += QStringLiteral("- Build Type: %1\n").arg(QString::fromStdString(context.buildType));
    md += QStringLiteral("- Compiler: %1\n").arg(QString::fromStdString(context.compiler));
    md += QStringLiteral("- Platform: %1\n\n").arg(QSysInfo::prettyProductName());

    md += QStringLiteral("## 2. Problem\n\n");
    md += QStringLiteral("- Title: %1\n").arg(QString::fromStdString(context.title));
    md += QStringLiteral("- Description: %1\n\n").arg(QString::fromStdString(context.description));

    md += QStringLiteral("## 3. Input Shapes\n\n");
    md += QStringLiteral("| ID | Type | Tolerance | BBox valid |\n");
    md += QStringLiteral("|---|---|---|---|\n");
    for (const ShapeNode& n : document.Nodes())
    {
        const QString bboxOk = n.bbox.IsVoid() ? QStringLiteral("void") : QStringLiteral("yes");
        md += QStringLiteral("| %1 | %2 | %3 | %4 |\n")
                  .arg(n.id)
                  .arg(QString::fromLatin1(ShapeKindDisplayName(n.kind)))
                  .arg(n.tolerance, 0, 'g', 12)
                  .arg(bboxOk);
    }
    md += QStringLiteral("\n");

    md += QStringLiteral("## 4. Diagnostic Findings\n\n");
    if (findings.empty())
    {
        md += QStringLiteral("_No findings._\n\n");
    }
    for (const DiagnosticFinding& f : findings)
    {
        md += QStringLiteral("### %1 — %2\n\n")
                  .arg(QString::fromStdString(f.ruleId), QString::fromStdString(f.title));
        md += QStringLiteral("**Severity:** %1\n\n").arg(QString::fromLatin1(severityString(f.severity)));
        md += QStringLiteral("%1\n\n").arg(QString::fromStdString(f.description));
        if (!f.evidence.empty())
        {
            md += QStringLiteral("**Evidence:**\n");
            for (const std::string& e : f.evidence)
            {
                md += QStringLiteral("- %1\n").arg(QString::fromStdString(e));
            }
            md += QStringLiteral("\n");
        }
        if (!f.suggestions.empty())
        {
            md += QStringLiteral("**Suggestions:**\n");
            for (const std::string& s : f.suggestions)
            {
                md += QStringLiteral("- %1\n").arg(QString::fromStdString(s));
            }
            md += QStringLiteral("\n");
        }
    }

    md += QStringLiteral("## 5. Suggested Next Steps\n\n");
    md += QStringLiteral("- Re-run diagnostics after geometry fixes.\n");
    md += QStringLiteral("- Export shape tree JSON for external diff tools.\n");
    return md;
}
