#pragma once

#include "core/case/CaseManifest.h"
#include "core/runner/CommandRunner.h"
#include "core/verify/VerificationResultParser.h"

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace occtdebug
{
struct TwoStagePhaseResultInput
{
    QString caseId;
    QString phase;
    QString note;
    QString workspaceRoot;
    CommandResult gateResult;
    bool gatePassed = false;
    QString gateStdoutLog;
    QString gateStderrLog;
    bool commandExecuted = false;
    CommandResult commandResult;
    QString commandStdoutLog;
    QString commandStderrLog;
    QString testgridSummaryPath;
    QString testdiffSummaryPath;
    QVector<TestgridRow> testgridRows;
    TestdiffSummary testdiff;
    QVector<VerificationFailureDetail> failureDetails;
    VerificationTimingSummary timing;
};

struct TwoStageWorkflowResultInput
{
    QString caseId;
    QString status;
    QString note;
    QString workspaceRoot;
    VerificationPlan plan;
    bool patchApplied = false;
    QVector<TestgridRow> testgridRows;
    TestdiffSummary testdiff;
    QVector<VerificationFailureDetail> failureDetails;
    VerificationTimingSummary timing;
    QJsonObject testdiffArtifacts;
    TestgridComparison beforeAfter;
    QString beforeSummaryPath;
    QString afterSummaryPath;
};

class TwoStageVerificationResultWriter
{
public:
    static QJsonObject buildPhaseResult(const TwoStagePhaseResultInput& input);
    static QJsonObject buildWorkflowResult(const TwoStageWorkflowResultInput& input);

    static QJsonArray testgridRowsToJson(const QVector<TestgridRow>& rows);
    static QJsonArray testdiffEntriesToJson(const TestdiffSummary& testdiff);
    static QJsonArray failureDetailsToJson(const QVector<VerificationFailureDetail>& details);
    static QJsonObject timingSummaryToJson(const VerificationTimingSummary& timing);
    static QJsonObject testdiffArtifactsToJson(const QString& summaryPath,
                                               const QString& workspaceRoot,
                                               const QString& commandStdout = {},
                                               const QString& commandStderr = {},
                                               int entriesCount = 0,
                                               int changedCount = 0,
                                               int failedCount = 0);
    static QJsonObject comparisonToJson(const TestgridComparison& comparison,
                                        const QString& beforePath,
                                        const QString& afterPath,
                                        const QString& workspaceRoot);
};
} // namespace occtdebug
