#include "workbench/ReportRefreshCoordinator.h"

#include "core/evidence/EvidenceBundleWriter.h"
#include "core/verify/VerificationReportWriter.h"

#include <QDir>

namespace occtdebug
{
namespace
{
void appendError(ReportRefreshResult& result, const QString& prefix, const QString& error)
{
    result.success = false;
    result.errors.push_back(QStringLiteral("[%1] %2").arg(prefix, error));
}
} // namespace

ReportRefreshResult ReportRefreshCoordinator::refresh(const CaseManifest& manifest,
                                                      const ReportRefreshPaths& paths,
                                                      const ReportRefreshRequest& request)
{
    ReportRefreshResult result;
    if (paths.workspaceRoot.trimmed().isEmpty())
    {
        return result;
    }

    if (request.writeEvidenceBundle)
    {
        result.evidenceBundlePath =
            QDir(paths.artifactDirectory).filePath(QStringLiteral("evidence_bundle.json"));
        QString error;
        if (EvidenceBundleWriter::writeBundle(manifest, paths.workspaceRoot, result.evidenceBundlePath, &error))
        {
            result.evidenceBundleWritten = true;
        }
        else
        {
            appendError(result, QStringLiteral("evidence"), error);
        }
    }

    if (request.writeVerificationReport)
    {
        result.verificationMarkdownPath =
            QDir(paths.reportDirectory).filePath(QStringLiteral("verification_report.md"));
        result.verificationJsonPath =
            QDir(paths.verificationDirectory).filePath(QStringLiteral("verification_report.json"));
        QString error;
        if (VerificationReportWriter::writeReport(
                manifest, paths.workspaceRoot, result.verificationMarkdownPath, result.verificationJsonPath, &error))
        {
            result.verificationReportWritten = true;
        }
        else
        {
            appendError(result, QStringLiteral("verify"), error);
        }
    }

    return result;
}
} // namespace occtdebug
