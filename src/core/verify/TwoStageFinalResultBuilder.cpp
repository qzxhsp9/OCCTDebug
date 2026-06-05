#include "core/verify/TwoStageFinalResultBuilder.h"

#include "core/verify/TestgridArtifactService.h"
#include "core/verify/VerificationWorkflow.h"
#include "core/verify/TwoStageVerificationResultWriter.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonObject>

namespace occtdebug
{
namespace
{
void appendFailureDetails(QVector<VerificationFailureDetail>& out, const QVector<VerificationFailureDetail>& next)
{
    for (const VerificationFailureDetail& detail : next)
    {
        out.push_back(detail);
    }
}

QString commandStatus(const CommandResult& result)
{
    if (result.timedOut)
    {
        return QStringLiteral("timed_out");
    }
    if (result.canceled)
    {
        return QStringLiteral("canceled");
    }
    return VerificationWorkflow::commandSucceeded(result) ? QStringLiteral("passed") : QStringLiteral("failed");
}
} // namespace

TwoStageFinalResultBuilderResult TwoStageFinalResultBuilder::build(const TwoStageFinalResultBuilderInput& input)
{
    TwoStageFinalResultBuilderResult result;
    result.beforeCommandExecuted = input.beforeCommandExecuted;
    result.afterCommandExecuted = input.afterCommandExecuted;

    const QString beforeSummaryPath =
        TestgridArtifactService::verificationPath(input.workspaceRoot, QStringLiteral("testgrid_before.txt"));
    const QString afterSummaryPath =
        TestgridArtifactService::verificationPath(input.workspaceRoot, QStringLiteral("testgrid_after.txt"));
    const QVector<TestgridRow> beforeRows =
        TestgridArtifactService::readPhaseRows(input.workspaceRoot, QStringLiteral("before"));
    result.afterRows = TestgridArtifactService::readPhaseRows(input.workspaceRoot, QStringLiteral("after"));
    if (result.afterRows.isEmpty())
    {
        result.afterRows = beforeRows;
    }

    result.comparison = VerificationResultParser::compareTestgridRows(beforeRows, result.afterRows);

    if (input.afterCommandExecuted)
    {
        const QString commandText = input.afterCommandResult.stdoutText
            + QLatin1Char('\n')
            + input.afterCommandResult.stderrText;
        result.testdiff = VerificationResultParser::parseTestdiffText(commandText);
    }
    const QString testdiffSummaryPath =
        TestgridArtifactService::verificationPath(input.workspaceRoot, QStringLiteral("testdiff_summary.txt"));
    if (result.testdiff.entries.isEmpty())
    {
        result.testdiff = VerificationResultParser::parseTestdiffText(
            TestgridArtifactService::readTextArtifact(testdiffSummaryPath));
    }

    const QString testdiffArtifact = QFileInfo::exists(testdiffSummaryPath)
        ? QDir(input.workspaceRoot).relativeFilePath(testdiffSummaryPath)
        : QString();
    appendFailureDetails(result.failureDetails,
                         VerificationResultParser::failureDetailsForTestgridRows(result.afterRows, QStringLiteral("artifacts/testgrid_result.json")));
    appendFailureDetails(result.failureDetails,
                         VerificationResultParser::failureDetailsForTestdiff(result.testdiff, testdiffArtifact));
    appendFailureDetails(result.failureDetails,
                         VerificationResultParser::failureDetailsForComparison(result.comparison, QStringLiteral("artifacts/testgrid_result.json")));

    QVector<VerificationTimingEntry> timingEntries;
    timingEntries.push_back({
        QStringLiteral("before_draw_smoke_gate"),
        input.beforeGateResult.elapsedMs,
        commandStatus(input.beforeGateResult),
    });
    if (input.beforeCommandExecuted)
    {
        timingEntries.push_back({
            QStringLiteral("before_configured_testgrid"),
            input.beforeCommandResult.elapsedMs,
            commandStatus(input.beforeCommandResult),
        });
    }
    timingEntries.push_back({
        QStringLiteral("after_draw_smoke_gate"),
        input.afterGateResult.elapsedMs,
        commandStatus(input.afterGateResult),
    });
    if (input.afterCommandExecuted)
    {
        timingEntries.push_back({
            QStringLiteral("after_configured_testgrid"),
            input.afterCommandResult.elapsedMs,
            commandStatus(input.afterCommandResult),
        });
    }
    result.timing = VerificationResultParser::timingSummary(timingEntries);

    QJsonObject testdiffArtifacts = TwoStageVerificationResultWriter::testdiffArtifactsToJson(
        testdiffSummaryPath,
        input.workspaceRoot,
        input.afterCommandExecuted ? QStringLiteral("logs/testgrid_after.stdout.log") : QString(),
        input.afterCommandExecuted ? QStringLiteral("logs/testgrid_after.stderr.log") : QString(),
        result.testdiff.entries.size(),
        result.testdiff.changedCount,
        result.testdiff.failedCount);
    testdiffArtifacts.insert(QStringLiteral("before_result"), QStringLiteral("artifacts/testgrid_before_result.json"));
    testdiffArtifacts.insert(QStringLiteral("after_result"), QStringLiteral("artifacts/testgrid_after_result.json"));
    testdiffArtifacts.insert(QStringLiteral("two_stage_result"), QStringLiteral("artifacts/testgrid_two_stage_result.json"));

    result.writerInput = {
        input.caseId,
        input.workspaceRoot,
        input.finalStatus,
        input.note,
        input.plan,
        input.patchApplied,
        result.afterRows,
        result.testdiff,
        result.failureDetails,
        result.timing,
        testdiffArtifacts,
        result.comparison,
        QFileInfo::exists(beforeSummaryPath) ? beforeSummaryPath : QString(),
        QFileInfo::exists(afterSummaryPath) ? afterSummaryPath : QString(),
    };
    return result;
}
} // namespace occtdebug
