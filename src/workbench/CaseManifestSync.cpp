#include "workbench/CaseManifestSync.h"

#include "workbench/WorkbenchMockData.h"

namespace occtdebug
{
void CaseManifestSync::syncMutableFields(CaseManifest& manifest, const WorkbenchMockData& data)
{
    manifest.reproScript = data.reproScript;
    manifest.reproStatus = data.reproStatus;
    manifest.environmentSummary = data.environmentSummary;
    manifest.geometrySummary = data.geometrySummary;
    manifest.geometryChecks = data.geometryChecks;
    manifest.evidenceItems = data.evidenceItems;
    manifest.verificationItems = data.verificationItems;
    manifest.verificationPlan = data.verificationPlan;
    manifest.testdiffGenerationConfig = data.testdiffGenerationConfig;
    manifest.taskHistory = data.taskHistory;
    manifest.patchReviewStatus = data.patchReviewStatus;
    manifest.patchWorktreeRoot = data.patchWorktreeRoot;
    manifest.patchApplyStatus = data.patchApplyStatus;
    manifest.patchApplyLog = data.patchApplyLog;
    manifest.patchSignoffStatus = data.patchSignoffStatus;
    manifest.patchSignoffNote = data.patchSignoffNote;
    manifest.patchReviewItems = data.patchReviewItems;
    manifest.testgridRows = data.testgridRows;

    if (manifest.workflowState.steps.isEmpty())
    {
        manifest.workflowState.steps = data.workflowSteps;
    }
    if (manifest.workflowSteps.isEmpty())
    {
        manifest.workflowSteps = manifest.workflowState.steps;
    }
    if (manifest.workflowState.activeStepId.isEmpty() && !manifest.workflowState.steps.isEmpty())
    {
        manifest.workflowState.activeStepId = manifest.workflowState.steps.first().id;
    }
}
} // namespace occtdebug
