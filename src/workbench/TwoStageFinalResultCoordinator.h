#pragma once

#include "core/case/CaseManifest.h"
#include "core/verify/VerificationResultParser.h"
#include "workbench/WorkbenchMockData.h"

namespace occtdebug
{
struct TwoStageFinalResultSyncInput
{
    QString finalStatus;
    QString note;
    bool beforeCommandExecuted = false;
    bool afterCommandExecuted = false;
    QVector<TestgridRow> afterRows;
    TestgridComparison comparison;
    TestdiffSummary testdiff;
    QVector<VerificationFailureDetail> failureDetails;
    VerificationTimingSummary timing;
};

struct TwoStageFinalResultSyncResult
{
    QString diffSummary;
    QVector<LabelValue> verificationItems;
    EvidenceRecord evidence;
    bool saveCaseManifest = true;
    bool writeEvidenceBundle = true;
    bool writeVerificationReport = true;
};

class TwoStageFinalResultCoordinator
{
public:
    static TwoStageFinalResultSyncResult sync(WorkbenchMockData& data,
                                             const TwoStageFinalResultSyncInput& input);
};
} // namespace occtdebug
