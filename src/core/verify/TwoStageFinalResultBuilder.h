#pragma once

#include "core/case/CaseManifest.h"
#include "core/runner/CommandRunner.h"
#include "core/verify/TwoStageFinalResultWriter.h"

namespace occtdebug
{
struct TwoStageFinalResultBuilderInput
{
    QString caseId;
    QString workspaceRoot;
    QString finalStatus;
    QString note;
    VerificationPlan plan;
    bool patchApplied = false;
    bool beforeCommandExecuted = false;
    bool afterCommandExecuted = false;
    CommandResult beforeGateResult;
    CommandResult beforeCommandResult;
    CommandResult afterGateResult;
    CommandResult afterCommandResult;
};

struct TwoStageFinalResultBuilderResult
{
    TwoStageFinalResultWriterInput writerInput;
    bool beforeCommandExecuted = false;
    bool afterCommandExecuted = false;
    QVector<TestgridRow> afterRows;
    TestgridComparison comparison;
    TestdiffSummary testdiff;
    QVector<VerificationFailureDetail> failureDetails;
    VerificationTimingSummary timing;
};

class TwoStageFinalResultBuilder
{
public:
    static TwoStageFinalResultBuilderResult build(const TwoStageFinalResultBuilderInput& input);
};
} // namespace occtdebug
