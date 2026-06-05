#pragma once

#include "core/case/CaseManifest.h"
#include "core/verify/TestdiffAdapterResultWriter.h"
#include "workbench/WorkbenchMockData.h"

namespace occtdebug
{
struct TestdiffAdapterResultSyncResult
{
    LabelValue verificationItem;
    EvidenceRecord evidence;
    bool saveCaseManifest = true;
    bool writeEvidenceBundle = true;
    bool writeVerificationReport = true;
};

class TestdiffAdapterResultCoordinator
{
public:
    static TestdiffAdapterResultSyncResult sync(WorkbenchMockData& data,
                                                const TestdiffAdapterResultWriterResult& writerResult);
};
} // namespace occtdebug
