#include "core/verify/TwoStagePhaseResultWriter.h"

#include "core/verify/TestgridArtifactService.h"
#include "core/verify/TwoStageVerificationResultWriter.h"

#include <QDir>
#include <QFileInfo>
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
} // namespace

bool TwoStagePhaseResultWriter::writePhaseResult(const TwoStagePhaseResultWriterInput& input,
                                                 TwoStagePhaseResultWriterResult* result,
                                                 QString* error)
{
    if (result == nullptr)
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("two-stage phase writer result pointer is null");
        }
        return false;
    }

    TwoStagePhaseResultWriterResult out;
    out.phase = input.phase;

    if (input.workspaceRoot.trimmed().isEmpty())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("two-stage phase writer workspace root is empty");
        }
        return false;
    }
    if (input.phase.trimmed().isEmpty())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("two-stage phase writer phase is empty");
        }
        return false;
    }
    if (!TestgridArtifactService::ensureWorkspaceDirectories(input.workspaceRoot, error))
    {
        return false;
    }

    const QString gateStdoutName = QStringLiteral("testgrid_%1_gate.stdout.log").arg(input.phase);
    const QString gateStderrName = QStringLiteral("testgrid_%1_gate.stderr.log").arg(input.phase);
    if (!TestgridArtifactService::writeCommandLogs(input.workspaceRoot,
                                                   gateStdoutName,
                                                   gateStderrName,
                                                   input.gateResult,
                                                   error))
    {
        return false;
    }
    out.gateStdoutRelativePath = TestgridArtifactService::logRelativePath(gateStdoutName);
    out.gateStderrRelativePath = TestgridArtifactService::logRelativePath(gateStderrName);

    if (input.commandExecuted)
    {
        const QString commandStdoutName = QStringLiteral("testgrid_%1.stdout.log").arg(input.phase);
        const QString commandStderrName = QStringLiteral("testgrid_%1.stderr.log").arg(input.phase);
        if (!TestgridArtifactService::writeCommandLogs(input.workspaceRoot,
                                                       commandStdoutName,
                                                       commandStderrName,
                                                       input.commandResult,
                                                       error))
        {
            return false;
        }
        out.commandStdoutRelativePath = TestgridArtifactService::logRelativePath(commandStdoutName);
        out.commandStderrRelativePath = TestgridArtifactService::logRelativePath(commandStderrName);
    }

    const bool gatePassed = commandSucceeded(input.gateResult);
    const QString phaseSummaryPath =
        TestgridArtifactService::verificationPath(input.workspaceRoot, QStringLiteral("testgrid_%1.txt").arg(input.phase));
    const QString summaryPath =
        TestgridArtifactService::verificationPath(input.workspaceRoot, QStringLiteral("testgrid_summary.txt"));
    const QString testdiffPath =
        TestgridArtifactService::verificationPath(input.workspaceRoot, QStringLiteral("testdiff_summary.txt"));

    if (gatePassed)
    {
        if (input.commandExecuted)
        {
            const QString commandText = input.commandResult.stdoutText + QLatin1Char('\n') + input.commandResult.stderrText;
            out.rows = VerificationResultParser::parseTestgridText(commandText);
            out.testdiff = VerificationResultParser::parseTestdiffText(commandText);
        }
        if (out.rows.isEmpty())
        {
            out.rows = VerificationResultParser::parseTestgridText(TestgridArtifactService::readTextArtifact(phaseSummaryPath));
        }
        if (out.rows.isEmpty())
        {
            out.rows = VerificationResultParser::parseTestgridText(TestgridArtifactService::readTextArtifact(summaryPath));
        }
        if (out.testdiff.entries.isEmpty())
        {
            out.testdiff = VerificationResultParser::parseTestdiffText(TestgridArtifactService::readTextArtifact(testdiffPath));
        }
        if (out.rows.isEmpty())
        {
            if (input.commandExecuted && !commandSucceeded(input.commandResult))
            {
                out.rows.push_back({QStringLiteral("configured_testgrid_%1").arg(input.phase), QStringLiteral("1"), QStringLiteral("0"), QStringLiteral("1"), QStringLiteral("0%")});
            }
            else
            {
                out.rows.push_back({QStringLiteral("draw_smoke_gate_%1").arg(input.phase), QStringLiteral("1"), QStringLiteral("1"), QStringLiteral("0"), QStringLiteral("100%")});
            }
        }
    }
    else
    {
        out.rows.push_back({QStringLiteral("draw_smoke_gate_%1").arg(input.phase), QStringLiteral("1"), QStringLiteral("0"), QStringLiteral("1"), QStringLiteral("0%")});
    }

    if (!TestgridArtifactService::writePhaseSummary(input.workspaceRoot,
                                                    input.phase,
                                                    out.rows,
                                                    &out.phaseSummaryPath,
                                                    error))
    {
        return false;
    }
    out.phaseSummaryRelativePath = QDir(input.workspaceRoot).relativeFilePath(out.phaseSummaryPath);

    const QString phaseArtifact = TestgridArtifactService::artifactRelativePath(QStringLiteral("testgrid_%1_result.json").arg(input.phase));
    const QString testdiffArtifact = QFileInfo::exists(testdiffPath)
        ? QDir(input.workspaceRoot).relativeFilePath(testdiffPath)
        : QString();
    appendFailureDetails(out.failureDetails,
                         VerificationResultParser::failureDetailsForTestgridRows(out.rows, phaseArtifact));
    appendFailureDetails(out.failureDetails,
                         VerificationResultParser::failureDetailsForTestdiff(out.testdiff, testdiffArtifact));

    QVector<VerificationTimingEntry> timingEntries {
        {QStringLiteral("%1_draw_smoke_gate").arg(input.phase), input.gateResult.elapsedMs, commandStatus(input.gateResult)},
    };
    if (input.commandExecuted)
    {
        timingEntries.push_back({
            QStringLiteral("%1_configured_testgrid").arg(input.phase),
            input.commandResult.elapsedMs,
            commandStatus(input.commandResult),
        });
    }
    out.timing = VerificationResultParser::timingSummary(timingEntries);

    out.phaseResult = TwoStageVerificationResultWriter::buildPhaseResult({
        input.caseId,
        input.phase,
        input.note,
        input.workspaceRoot,
        input.gateResult,
        gatePassed,
        out.gateStdoutRelativePath,
        out.gateStderrRelativePath,
        input.commandExecuted,
        input.commandExecuted ? input.commandResult : CommandResult(),
        out.commandStdoutRelativePath,
        out.commandStderrRelativePath,
        out.phaseSummaryPath,
        testdiffPath,
        out.rows,
        out.testdiff,
        out.failureDetails,
        out.timing,
    });

    out.artifactPath =
        TestgridArtifactService::artifactPath(input.workspaceRoot, QStringLiteral("testgrid_%1_result.json").arg(input.phase));
    out.artifactRelativePath = phaseArtifact;
    if (!TestgridArtifactService::writeJsonArtifact(out.artifactPath, out.phaseResult, error))
    {
        return false;
    }

    out.status = gatePassed ? QStringLiteral("written") : QStringLiteral("gate_failed_written");
    *result = out;
    return true;
}
} // namespace occtdebug
