#include "core/verify/TwoStageFinalResultWriter.h"

#include "core/verify/TestgridArtifactService.h"
#include "core/verify/TwoStageVerificationResultWriter.h"

namespace occtdebug
{
bool TwoStageFinalResultWriter::writeFinalResult(const TwoStageFinalResultWriterInput& input,
                                                 TwoStageFinalResultWriterResult* out,
                                                 QString* error)
{
    TwoStageFinalResultWriterResult result;
    result.finalResult = TwoStageVerificationResultWriter::buildWorkflowResult({
        input.caseId,
        input.finalStatus,
        input.note,
        input.workspaceRoot,
        input.plan,
        input.patchApplied,
        input.afterRows,
        input.testdiff,
        input.failureDetails,
        input.timing,
        input.testdiffArtifacts,
        input.comparison,
        input.beforeSummaryPath,
        input.afterSummaryPath,
    });
    result.twoStageResultPath =
        TestgridArtifactService::artifactPath(input.workspaceRoot, QStringLiteral("testgrid_two_stage_result.json"));
    result.legacyResultPath =
        TestgridArtifactService::artifactPath(input.workspaceRoot, QStringLiteral("testgrid_result.json"));

    if (!TestgridArtifactService::writeJsonArtifact(result.twoStageResultPath, result.finalResult, error))
    {
        return false;
    }
    if (!TestgridArtifactService::writeJsonArtifact(result.legacyResultPath, result.finalResult, error))
    {
        return false;
    }
    if (out != nullptr)
    {
        *out = result;
    }
    return true;
}
} // namespace occtdebug
