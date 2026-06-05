#pragma once

#include "core/case/CaseManifest.h"
#include "core/runner/CommandRunner.h"
#include "core/verify/VerificationResultParser.h"

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace occtdebug
{
struct TwoStagePhaseResultWriterInput
{
    QString caseId;
    QString workspaceRoot;
    QString phase;
    QString note;
    CommandResult gateResult;
    bool commandExecuted = false;
    CommandResult commandResult;
};

struct TwoStagePhaseResultWriterResult
{
    QString status;
    QString phase;
    QString artifactPath;
    QString artifactRelativePath;
    QString phaseSummaryPath;
    QString phaseSummaryRelativePath;
    QString gateStdoutRelativePath;
    QString gateStderrRelativePath;
    QString commandStdoutRelativePath;
    QString commandStderrRelativePath;
    QVector<TestgridRow> rows;
    TestdiffSummary testdiff;
    QVector<VerificationFailureDetail> failureDetails;
    VerificationTimingSummary timing;
    QJsonObject phaseResult;
};

class TwoStagePhaseResultWriter
{
public:
    static bool writePhaseResult(const TwoStagePhaseResultWriterInput& input,
                                 TwoStagePhaseResultWriterResult* result,
                                 QString* error = nullptr);
};
} // namespace occtdebug
