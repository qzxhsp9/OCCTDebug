#include "core/verify/TwoStageVerificationResultWriter.h"

#include "core/verify/TestdiffArtifactScanner.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>

namespace occtdebug
{
namespace
{
QJsonObject planToJson(const VerificationPlan& plan)
{
    return {
        {QStringLiteral("testgrid_root"), plan.testgridRoot},
        {QStringLiteral("testgrid_executable"), plan.testgridExecutable},
        {QStringLiteral("testgrid_arguments"), plan.testgridArguments},
        {QStringLiteral("testgrid_group"), plan.testgridGroup},
        {QStringLiteral("testgrid_grid"), plan.testgridGrid},
        {QStringLiteral("testgrid_case"), plan.testgridCase},
        {QStringLiteral("testdiff_executable"), plan.testdiffExecutable},
        {QStringLiteral("testdiff_arguments"), plan.testdiffArguments},
        {QStringLiteral("testdiff_output_root"), plan.testdiffOutputRoot},
    };
}

QString relativeIfExists(const QString& workspaceRoot, const QString& path)
{
    if (path.isEmpty() || !QFileInfo::exists(path))
    {
        return QString();
    }
    return QDir(workspaceRoot).relativeFilePath(path);
}
} // namespace

QJsonObject TwoStageVerificationResultWriter::buildPhaseResult(const TwoStagePhaseResultInput& input)
{
    return {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("case_id"), input.caseId},
        {QStringLiteral("phase"), input.phase},
        {QStringLiteral("note"), input.note},
        {QStringLiteral("draw_smoke_gate_passed"), input.gatePassed},
        {QStringLiteral("gate"), QJsonObject {
             {QStringLiteral("program"), input.gateResult.program},
             {QStringLiteral("arguments"), input.gateResult.arguments.join(QLatin1Char(' '))},
             {QStringLiteral("working_directory"), input.gateResult.workingDirectory},
             {QStringLiteral("exit_code"), input.gateResult.exitCode},
             {QStringLiteral("elapsed_ms"), static_cast<double>(input.gateResult.elapsedMs)},
             {QStringLiteral("canceled"), input.gateResult.canceled},
             {QStringLiteral("timed_out"), input.gateResult.timedOut},
             {QStringLiteral("timeout_ms"), input.gateResult.timeoutMs},
             {QStringLiteral("stdout"), input.gateStdoutLog},
             {QStringLiteral("stderr"), input.gateStderrLog},
         }},
        {QStringLiteral("testgrid_command"), QJsonObject {
             {QStringLiteral("executed"), input.commandExecuted},
             {QStringLiteral("program"), input.commandExecuted ? input.commandResult.program : QString()},
             {QStringLiteral("arguments"), input.commandExecuted ? input.commandResult.arguments.join(QLatin1Char(' ')) : QString()},
             {QStringLiteral("working_directory"), input.commandExecuted ? input.commandResult.workingDirectory : QString()},
             {QStringLiteral("exit_code"), input.commandExecuted ? input.commandResult.exitCode : -1},
             {QStringLiteral("elapsed_ms"), input.commandExecuted ? static_cast<double>(input.commandResult.elapsedMs) : 0.0},
             {QStringLiteral("canceled"), input.commandExecuted && input.commandResult.canceled},
             {QStringLiteral("timed_out"), input.commandExecuted && input.commandResult.timedOut},
             {QStringLiteral("timeout_ms"), input.commandExecuted ? input.commandResult.timeoutMs : 0},
             {QStringLiteral("stdout"), input.commandStdoutLog},
             {QStringLiteral("stderr"), input.commandStderrLog},
         }},
        {QStringLiteral("testgrid_summary"), relativeIfExists(input.workspaceRoot, input.testgridSummaryPath)},
        {QStringLiteral("testgrid_rows"), testgridRowsToJson(input.testgridRows)},
        {QStringLiteral("testdiff_entries"), testdiffEntriesToJson(input.testdiff)},
        {QStringLiteral("failure_details"), failureDetailsToJson(input.failureDetails)},
        {QStringLiteral("timing"), timingSummaryToJson(input.timing)},
        {QStringLiteral("testdiff_artifacts"), testdiffArtifactsToJson(input.testdiffSummaryPath,
             input.workspaceRoot,
             input.commandStdoutLog,
             input.commandStderrLog,
             input.testdiff.entries.size(),
             input.testdiff.changedCount,
             input.testdiff.failedCount)},
    };
}

QJsonObject TwoStageVerificationResultWriter::buildWorkflowResult(const TwoStageWorkflowResultInput& input)
{
    return {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("case_id"), input.caseId},
        {QStringLiteral("mode"), QStringLiteral("two_stage")},
        {QStringLiteral("status"), input.status},
        {QStringLiteral("note"), input.note},
        {QStringLiteral("testgrid_plan"), planToJson(input.plan)},
        {QStringLiteral("phases"), QJsonObject {
             {QStringLiteral("before"), QStringLiteral("artifacts/testgrid_before_result.json")},
             {QStringLiteral("after"), QStringLiteral("artifacts/testgrid_after_result.json")},
         }},
        {QStringLiteral("patch_applied"), input.patchApplied},
        {QStringLiteral("testgrid_rows"), testgridRowsToJson(input.testgridRows)},
        {QStringLiteral("testdiff_entries"), testdiffEntriesToJson(input.testdiff)},
        {QStringLiteral("failure_details"), failureDetailsToJson(input.failureDetails)},
        {QStringLiteral("timing"), timingSummaryToJson(input.timing)},
        {QStringLiteral("testdiff_artifacts"), input.testdiffArtifacts},
        {QStringLiteral("before_after"), comparisonToJson(input.beforeAfter,
             input.beforeSummaryPath,
             input.afterSummaryPath,
             input.workspaceRoot)},
    };
}

QJsonArray TwoStageVerificationResultWriter::testgridRowsToJson(const QVector<TestgridRow>& rows)
{
    QJsonArray out;
    for (const TestgridRow& row : rows)
    {
        out.append(QJsonObject {
            {QStringLiteral("module"), row.module},
            {QStringLiteral("run_count"), row.runCount},
            {QStringLiteral("pass_count"), row.passCount},
            {QStringLiteral("fail_count"), row.failCount},
            {QStringLiteral("pass_rate"), row.passRate},
        });
    }
    return out;
}

QJsonArray TwoStageVerificationResultWriter::testdiffEntriesToJson(const TestdiffSummary& testdiff)
{
    QJsonArray out;
    for (const TestdiffEntry& entry : testdiff.entries)
    {
        out.append(QJsonObject {
            {QStringLiteral("name"), entry.name},
            {QStringLiteral("status"), entry.status},
            {QStringLiteral("metric"), entry.metric},
            {QStringLiteral("note"), entry.note},
        });
    }
    return out;
}

QJsonArray TwoStageVerificationResultWriter::failureDetailsToJson(const QVector<VerificationFailureDetail>& details)
{
    QJsonArray out;
    for (const VerificationFailureDetail& detail : details)
    {
        QJsonObject item {
            {QStringLiteral("type"), detail.type},
            {QStringLiteral("name"), detail.name},
            {QStringLiteral("status"), detail.status},
            {QStringLiteral("summary"), detail.summary},
        };
        if (!detail.artifact.isEmpty())
        {
            item.insert(QStringLiteral("artifact"), detail.artifact);
        }
        out.append(item);
    }
    return out;
}

QJsonObject TwoStageVerificationResultWriter::timingSummaryToJson(const VerificationTimingSummary& timing)
{
    QJsonArray entries;
    for (const VerificationTimingEntry& entry : timing.entries)
    {
        entries.append(QJsonObject {
            {QStringLiteral("name"), entry.name},
            {QStringLiteral("elapsed_ms"), static_cast<double>(entry.elapsedMs)},
            {QStringLiteral("status"), entry.status},
        });
    }
    return {
        {QStringLiteral("total_elapsed_ms"), static_cast<double>(timing.totalElapsedMs)},
        {QStringLiteral("summary"), timing.summaryText()},
        {QStringLiteral("entries"), entries},
    };
}

QJsonObject TwoStageVerificationResultWriter::testdiffArtifactsToJson(const QString& summaryPath,
                                                                       const QString& workspaceRoot,
                                                                       const QString& commandStdout,
                                                                       const QString& commandStderr,
                                                                       int entriesCount,
                                                                       int changedCount,
                                                                       int failedCount)
{
    return TestdiffArtifactScanner::buildManifest(summaryPath,
                                                  workspaceRoot,
                                                  commandStdout,
                                                  commandStderr,
                                                  entriesCount,
                                                  changedCount,
                                                  failedCount);
}

QJsonObject TwoStageVerificationResultWriter::comparisonToJson(const TestgridComparison& comparison,
                                                               const QString& beforePath,
                                                               const QString& afterPath,
                                                               const QString& workspaceRoot)
{
    QJsonArray rows;
    for (const TestgridComparisonRow& row : comparison.rows)
    {
        rows.append(QJsonObject {
            {QStringLiteral("module"), row.module},
            {QStringLiteral("before_run_count"), row.beforeRunCount},
            {QStringLiteral("before_pass_count"), row.beforePassCount},
            {QStringLiteral("before_fail_count"), row.beforeFailCount},
            {QStringLiteral("after_run_count"), row.afterRunCount},
            {QStringLiteral("after_pass_count"), row.afterPassCount},
            {QStringLiteral("after_fail_count"), row.afterFailCount},
            {QStringLiteral("pass_delta"), row.passDelta},
            {QStringLiteral("fail_delta"), row.failDelta},
            {QStringLiteral("status"), row.status},
        });
    }

    const bool available = comparison.isAvailable();
    return {
        {QStringLiteral("available"), available},
        {QStringLiteral("status"), !available ? QStringLiteral("unavailable") : (comparison.hasRegression() ? QStringLiteral("regressed") : QStringLiteral("not_regressed"))},
        {QStringLiteral("summary"), comparison.summaryText()},
        {QStringLiteral("before_run_total"), comparison.beforeRunTotal},
        {QStringLiteral("before_pass_total"), comparison.beforePassTotal},
        {QStringLiteral("before_fail_total"), comparison.beforeFailTotal},
        {QStringLiteral("after_run_total"), comparison.afterRunTotal},
        {QStringLiteral("after_pass_total"), comparison.afterPassTotal},
        {QStringLiteral("after_fail_total"), comparison.afterFailTotal},
        {QStringLiteral("pass_delta"), comparison.passDelta},
        {QStringLiteral("fail_delta"), comparison.failDelta},
        {QStringLiteral("before_summary"), relativeIfExists(workspaceRoot, beforePath)},
        {QStringLiteral("after_summary"), relativeIfExists(workspaceRoot, afterPath)},
        {QStringLiteral("rows"), rows},
    };
}
} // namespace occtdebug
