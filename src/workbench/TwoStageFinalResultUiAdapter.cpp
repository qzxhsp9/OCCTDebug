#include "workbench/TwoStageFinalResultUiAdapter.h"

#include "workbench/EvidencePanel.h"
#include "workbench/TestgridTablePresenter.h"
#include "workbench/VerificationPanel.h"

#include <QLabel>

namespace occtdebug
{
TwoStageFinalResultUiActions TwoStageFinalResultUiAdapter::apply(
    const TwoStageFinalResultUiTargets& targets,
    const WorkbenchMockData& data,
    const TwoStageFinalResultSyncResult& syncResult)
{
    if (targets.diffLabel != nullptr)
    {
        targets.diffLabel->setText(data.diffSummary);
    }
    if (targets.verificationPanel != nullptr)
    {
        targets.verificationPanel->setItems(data.verificationItems);
    }
    if (targets.evidencePanel != nullptr)
    {
        targets.evidencePanel->appendRecord(syncResult.evidence);
    }
    TestgridTablePresenter::applyToTable(targets.testgridTable, data.testgridRows);

    return {
        true,
        syncResult.saveCaseManifest,
        syncResult.writeEvidenceBundle,
        syncResult.writeVerificationReport,
    };
}
} // namespace occtdebug
