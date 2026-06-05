#include "workbench/ReportRefreshCoordinator.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

namespace
{
bool expect(bool condition, const char* message)
{
    if (!condition)
    {
        QTextStream(stderr) << message << "\n";
    }
    return condition;
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir workspace;
    if (!expect(workspace.isValid(), "temporary workspace is invalid"))
    {
        return 1;
    }

    QDir root(workspace.path());
    if (!expect(root.mkpath(QStringLiteral("artifacts")), "artifact directory failed")
        || !expect(root.mkpath(QStringLiteral("report")), "report directory failed")
        || !expect(root.mkpath(QStringLiteral("verification")), "verification directory failed"))
    {
        return 2;
    }

    occtdebug::CaseManifest manifest;
    manifest.caseId = QStringLiteral("OCC-REPORT-REFRESH");
    manifest.title = QStringLiteral("Report refresh coordinator smoke");
    manifest.status = QStringLiteral("investigating");
    manifest.evidenceItems.push_back({
        QStringLiteral("DRAW"),
        QStringLiteral("Smoke evidence"),
        QStringLiteral("minimal evidence item"),
        QStringLiteral("logs/draw.stdout.log"),
    });
    manifest.verificationItems.push_back({
        QStringLiteral("draw smoke"),
        QStringLiteral("passed"),
    });

    const occtdebug::ReportRefreshResult result = occtdebug::ReportRefreshCoordinator::refresh(
        manifest,
        {
            workspace.path(),
            root.filePath(QStringLiteral("artifacts")),
            root.filePath(QStringLiteral("report")),
            root.filePath(QStringLiteral("verification")),
        },
        {
            true,
            true,
            QStringLiteral("smoke"),
        });

    if (!expect(result.success, "refresh failed")
        || !expect(result.evidenceBundleWritten, "evidence bundle was not written")
        || !expect(result.verificationReportWritten, "verification report was not written")
        || !expect(QFile::exists(result.evidenceBundlePath), "evidence bundle path missing")
        || !expect(QFile::exists(result.verificationMarkdownPath), "verification markdown path missing")
        || !expect(QFile::exists(result.verificationJsonPath), "verification json path missing")
        || !expect(result.evidenceBundlePath.startsWith(workspace.path()), "evidence path outside workspace")
        || !expect(result.verificationMarkdownPath.startsWith(workspace.path()), "markdown path outside workspace")
        || !expect(result.verificationJsonPath.startsWith(workspace.path()), "json path outside workspace"))
    {
        for (const QString& error : result.errors)
        {
            QTextStream(stderr) << error << "\n";
        }
        return 3;
    }

    QTextStream(stdout) << "REPORT_REFRESH_COORDINATOR_SMOKE_OK\n";
    return 0;
}
