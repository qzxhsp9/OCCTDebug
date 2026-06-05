#include "core/verify/TestgridResultWriter.h"

#include "core/verify/TestgridArtifactService.h"
#include "core/verify/TwoStageVerificationResultWriter.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QProcess>

namespace occtdebug
{
namespace
{
bool commandSucceeded(const CommandResult& result)
{
    return !result.canceled
        && !result.timedOut
        && result.exitCode == 0;
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
    return commandSucceeded(result) ? QStringLiteral("passed") : QStringLiteral("failed");
}

void appendFailureDetails(QVector<VerificationFailureDetail>& target,
                          const QVector<VerificationFailureDetail>& values)
{
    for (const VerificationFailureDetail& value : values)
    {
        target.push_back(value);
    }
}

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

QJsonObject commandToJson(bool executed, const CommandResult& result)
{
    QJsonObject json {
        {QStringLiteral("executed"), executed},
    };
    if (executed)
    {
        json.insert(QStringLiteral("program"), result.program);
        json.insert(QStringLiteral("arguments"), result.arguments.join(QLatin1Char(' ')));
        json.insert(QStringLiteral("working_directory"), result.workingDirectory);
        json.insert(QStringLiteral("exit_code"), result.exitCode);
        json.insert(QStringLiteral("elapsed_ms"), static_cast<double>(result.elapsedMs));
        json.insert(QStringLiteral("canceled"), result.canceled);
        json.insert(QStringLiteral("timed_out"), result.timedOut);
        json.insert(QStringLiteral("timeout_ms"), result.timeoutMs);
        json.insert(QStringLiteral("stdout"), QStringLiteral("logs/testgrid.stdout.log"));
        json.insert(QStringLiteral("stderr"), QStringLiteral("logs/testgrid.stderr.log"));
    }
    return json;
}

QString relativeIfAvailable(const QString& workspaceRoot, const QString& path)
{
    if (path.isEmpty())
    {
        return QString();
    }
    return QDir(workspaceRoot).relativeFilePath(path);
}
} // namespace

TestgridResultWriterResult TestgridResultWriter::buildSingleStageResult(const TestgridResultWriterInput& input)
{
    TestgridResultWriterResult out;
    out.gatePassed = commandSucceeded(input.gateResult);
    out.commandExecuted = input.commandExecuted;
    out.effectiveNote = input.note;

    QString testgridSummaryPath;
    QString testdiffSummaryPath;
    if (out.gatePassed)
    {
        if (input.commandExecuted)
        {
            const QString commandText = input.commandResult.stdoutText + QLatin1Char('\n') + input.commandResult.stderrText;
            out.rows = VerificationResultParser::parseTestgridText(commandText);
            out.testdiff = VerificationResultParser::parseTestdiffText(commandText);
        }

        testgridSummaryPath = TestgridArtifactService::verificationPath(input.workspaceRoot, QStringLiteral("testgrid_summary.txt"));
        testdiffSummaryPath = TestgridArtifactService::verificationPath(input.workspaceRoot, QStringLiteral("testdiff_summary.txt"));
        if (out.rows.isEmpty())
        {
            out.rows = VerificationResultParser::parseTestgridText(TestgridArtifactService::readTextArtifact(testgridSummaryPath));
        }
        if (out.testdiff.entries.isEmpty())
        {
            out.testdiff = VerificationResultParser::parseTestdiffText(TestgridArtifactService::readTextArtifact(testdiffSummaryPath));
        }
        if (out.rows.isEmpty())
        {
            if (input.commandExecuted && !commandSucceeded(input.commandResult))
            {
                out.rows.push_back({QStringLiteral("configured_testgrid"), QStringLiteral("1"), QStringLiteral("0"), QStringLiteral("1"), QStringLiteral("0%")});
            }
            else
            {
                out.rows.push_back({QStringLiteral("draw_smoke_gate"), QStringLiteral("1"), QStringLiteral("1"), QStringLiteral("0"), QStringLiteral("100%")});
            }
            if (out.effectiveNote.isEmpty())
            {
                out.effectiveNote = QStringLiteral("draw_smoke gate passed; no parseable testgrid rows found");
            }
        }
    }
    else
    {
        out.rows.push_back({QStringLiteral("draw_smoke_gate"), QStringLiteral("1"), QStringLiteral("0"), QStringLiteral("1"), QStringLiteral("0%")});
        if (out.effectiveNote.isEmpty())
        {
            out.effectiveNote = QStringLiteral("draw_smoke gate failed; testgrid/testdiff summaries were not parsed");
        }
    }

    int runTotal = 0;
    int passTotal = 0;
    int failTotal = 0;
    for (const TestgridRow& row : out.rows)
    {
        runTotal += row.runCount.toInt();
        passTotal += row.passCount.toInt();
        failTotal += row.failCount.toInt();
    }

    const QString beforeSummaryPath = TestgridArtifactService::verificationPath(input.workspaceRoot, QStringLiteral("testgrid_before.txt"));
    const QString afterSummaryPath = TestgridArtifactService::verificationPath(input.workspaceRoot, QStringLiteral("testgrid_after.txt"));
    const QVector<TestgridRow> beforeRows =
        VerificationResultParser::parseTestgridText(TestgridArtifactService::readTextArtifact(beforeSummaryPath));
    QVector<TestgridRow> afterRows =
        VerificationResultParser::parseTestgridText(TestgridArtifactService::readTextArtifact(afterSummaryPath));
    if (afterRows.isEmpty())
    {
        afterRows = out.rows;
    }
    out.beforeAfter = VerificationResultParser::compareTestgridRows(beforeRows, afterRows);

    const QString testgridArtifact = QStringLiteral("artifacts/testgrid_result.json");
    const QString testdiffArtifact = QFileInfo::exists(testdiffSummaryPath)
        ? QDir(input.workspaceRoot).relativeFilePath(testdiffSummaryPath)
        : QString();
    appendFailureDetails(out.failureDetails,
                         VerificationResultParser::failureDetailsForTestgridRows(out.rows, testgridArtifact));
    appendFailureDetails(out.failureDetails,
                         VerificationResultParser::failureDetailsForTestdiff(out.testdiff, testdiffArtifact));
    appendFailureDetails(out.failureDetails,
                         VerificationResultParser::failureDetailsForComparison(out.beforeAfter, testgridArtifact));

    QVector<VerificationTimingEntry> timingEntries {
        {QStringLiteral("draw_smoke_gate"), input.gateResult.elapsedMs, commandStatus(input.gateResult)},
    };
    if (input.commandExecuted)
    {
        timingEntries.push_back({
            QStringLiteral("configured_testgrid"),
            input.commandResult.elapsedMs,
            commandStatus(input.commandResult),
        });
    }
    out.timing = VerificationResultParser::timingSummary(timingEntries);

    out.verificationItems = {
        {QStringLiteral("draw_smoke gate"), out.gatePassed ? QStringLiteral("passed") : QStringLiteral("failed")},
        {QStringLiteral("testgrid runner"), input.commandExecuted ? QStringLiteral("executed") : QStringLiteral("skipped")},
        {QStringLiteral("testgrid plan"), QStringLiteral("group=%1 grid=%2 case=%3")
            .arg(input.plan.testgridGroup.isEmpty() ? QStringLiteral("-") : input.plan.testgridGroup,
                 input.plan.testgridGrid.isEmpty() ? QStringLiteral("-") : input.plan.testgridGrid,
                 input.plan.testgridCase.isEmpty() ? QStringLiteral("-") : input.plan.testgridCase)},
        {QStringLiteral("testgrid"), QStringLiteral("%1 / %2 passed, %3 failed").arg(passTotal).arg(runTotal).arg(failTotal)},
        {QStringLiteral("before/after"), out.beforeAfter.summaryText()},
        {QStringLiteral("testdiff"), out.testdiff.entries.isEmpty() ? QStringLiteral("not available") : out.testdiff.summaryText()},
    };
    out.diffSummary = QStringLiteral("%1\n%2")
        .arg(out.beforeAfter.summaryText(),
             out.testdiff.entries.isEmpty() ? QStringLiteral("testdiff not available") : out.testdiff.summaryText());

    out.json = {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("case_id"), input.caseId},
        {QStringLiteral("draw_smoke_gate_passed"), out.gatePassed},
        {QStringLiteral("gate"), QJsonObject {
             {QStringLiteral("program"), input.gateResult.program},
             {QStringLiteral("arguments"), input.gateResult.arguments.join(QLatin1Char(' '))},
             {QStringLiteral("working_directory"), input.gateResult.workingDirectory},
             {QStringLiteral("exit_code"), input.gateResult.exitCode},
             {QStringLiteral("elapsed_ms"), static_cast<double>(input.gateResult.elapsedMs)},
             {QStringLiteral("stdout"), QStringLiteral("logs/testgrid_gate.stdout.log")},
             {QStringLiteral("stderr"), QStringLiteral("logs/testgrid_gate.stderr.log")},
         }},
        {QStringLiteral("testgrid_plan"), planToJson(input.plan)},
        {QStringLiteral("testgrid_command"), commandToJson(input.commandExecuted, input.commandResult)},
        {QStringLiteral("stdout"), QStringLiteral("logs/testgrid_gate.stdout.log")},
        {QStringLiteral("stderr"), QStringLiteral("logs/testgrid_gate.stderr.log")},
        {QStringLiteral("testgrid_summary"), testgridSummaryPath.isEmpty() ? QString() : relativeIfAvailable(input.workspaceRoot, testgridSummaryPath)},
        {QStringLiteral("testdiff_summary"), testdiffSummaryPath.isEmpty() ? QString() : relativeIfAvailable(input.workspaceRoot, testdiffSummaryPath)},
        {QStringLiteral("note"), out.effectiveNote},
        {QStringLiteral("testgrid_rows"), TwoStageVerificationResultWriter::testgridRowsToJson(out.rows)},
        {QStringLiteral("testdiff_entries"), TwoStageVerificationResultWriter::testdiffEntriesToJson(out.testdiff)},
        {QStringLiteral("failure_details"), TwoStageVerificationResultWriter::failureDetailsToJson(out.failureDetails)},
        {QStringLiteral("timing"), TwoStageVerificationResultWriter::timingSummaryToJson(out.timing)},
        {QStringLiteral("testdiff_artifacts"), TwoStageVerificationResultWriter::testdiffArtifactsToJson(testdiffSummaryPath,
             input.workspaceRoot,
             input.commandExecuted ? QStringLiteral("logs/testgrid.stdout.log") : QString(),
             input.commandExecuted ? QStringLiteral("logs/testgrid.stderr.log") : QString(),
             out.testdiff.entries.size(),
             out.testdiff.changedCount,
             out.testdiff.failedCount)},
        {QStringLiteral("before_after"), TwoStageVerificationResultWriter::comparisonToJson(out.beforeAfter,
             QFileInfo::exists(beforeSummaryPath) ? beforeSummaryPath : QString(),
             QFileInfo::exists(afterSummaryPath) ? afterSummaryPath : QString(),
             input.workspaceRoot)},
    };
    out.artifactPath = TestgridArtifactService::artifactPath(input.workspaceRoot, QStringLiteral("testgrid_result.json"));
    return out;
}

bool TestgridResultWriter::writeSingleStageResult(const TestgridResultWriterInput& input,
                                                  TestgridResultWriterResult* result,
                                                  QString* error)
{
    TestgridResultWriterResult built = buildSingleStageResult(input);
    if (!TestgridArtifactService::writeJsonArtifact(built.artifactPath, built.json, error))
    {
        return false;
    }
    if (result != nullptr)
    {
        *result = built;
    }
    return true;
}
} // namespace occtdebug
