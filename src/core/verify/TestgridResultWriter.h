#pragma once

#include "core/case/CaseManifest.h"
#include "core/runner/CommandRunner.h"
#include "core/verify/VerificationResultParser.h"

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace occtdebug
{
struct TestgridResultWriterInput
{
    QString caseId;
    QString workspaceRoot;
    QString note;
    VerificationPlan plan;
    CommandResult gateResult;
    bool commandExecuted = false;
    CommandResult commandResult;
};

struct TestgridResultWriterResult
{
    bool gatePassed = false;
    bool commandExecuted = false;
    QString effectiveNote;
    QVector<TestgridRow> rows;
    TestdiffSummary testdiff;
    TestgridComparison beforeAfter;
    QVector<VerificationFailureDetail> failureDetails;
    VerificationTimingSummary timing;
    QVector<LabelValue> verificationItems;
    QString diffSummary;
    QJsonObject json;
    QString artifactPath;
};

class TestgridResultWriter
{
public:
    static TestgridResultWriterResult buildSingleStageResult(const TestgridResultWriterInput& input);
    static bool writeSingleStageResult(const TestgridResultWriterInput& input,
                                       TestgridResultWriterResult* result,
                                       QString* error = nullptr);
};
} // namespace occtdebug
