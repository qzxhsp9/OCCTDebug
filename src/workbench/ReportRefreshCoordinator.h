#pragma once

#include "core/case/CaseManifest.h"

#include <QString>
#include <QStringList>

namespace occtdebug
{
struct ReportRefreshPaths
{
    QString workspaceRoot;
    QString artifactDirectory;
    QString reportDirectory;
    QString verificationDirectory;
};

struct ReportRefreshRequest
{
    bool writeEvidenceBundle = false;
    bool writeVerificationReport = false;
    QString reason;
};

struct ReportRefreshResult
{
    bool success = true;
    bool evidenceBundleWritten = false;
    bool verificationReportWritten = false;
    QString evidenceBundlePath;
    QString verificationMarkdownPath;
    QString verificationJsonPath;
    QStringList errors;
};

class ReportRefreshCoordinator
{
public:
    static ReportRefreshResult refresh(const CaseManifest& manifest,
                                       const ReportRefreshPaths& paths,
                                       const ReportRefreshRequest& request);
};
} // namespace occtdebug
