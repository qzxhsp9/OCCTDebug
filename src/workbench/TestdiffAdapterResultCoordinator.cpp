#include "workbench/TestdiffAdapterResultCoordinator.h"

#include "workbench/EvidenceCoordinator.h"

namespace occtdebug
{
namespace
{
void setVerificationMetric(WorkbenchMockData& data, const LabelValue& item)
{
    for (LabelValue& existing : data.verificationItems)
    {
        if (existing.label == item.label)
        {
            existing.value = item.value;
            data.manifest.verificationItems = data.verificationItems;
            return;
        }
    }
    data.verificationItems.push_back(item);
    data.manifest.verificationItems = data.verificationItems;
}
} // namespace

TestdiffAdapterResultSyncResult TestdiffAdapterResultCoordinator::sync(
    WorkbenchMockData& data,
    const TestdiffAdapterResultWriterResult& writerResult)
{
    TestdiffAdapterResultSyncResult result;
    result.verificationItem = {
        QStringLiteral("testdiff adapter"),
        QStringLiteral("%1: %2").arg(writerResult.status, writerResult.adapterStatus),
    };
    result.evidence = {
        QStringLiteral("Artifact"),
        QStringLiteral("Testdiff runner import"),
        QStringLiteral("%1; %2").arg(writerResult.status, writerResult.adapterStatus),
        QStringLiteral("artifacts/testdiff_adapter_result.json"),
    };

    data.diffSummary = writerResult.diffSummary;
    data.manifest.diffSummary = writerResult.diffSummary;
    setVerificationMetric(data, result.verificationItem);
    EvidenceCoordinator::appendRecord(data, result.evidence);
    return result;
}
} // namespace occtdebug
