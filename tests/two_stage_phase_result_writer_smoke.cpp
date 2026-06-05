#include "core/verify/TwoStagePhaseResultWriter.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTextStream>

namespace
{
bool expect(bool condition, const char* message)
{
    if (!condition)
    {
        QTextStream(stderr) << message << "\n";
    }
    return condition;
}

bool writeFile(const QString& path, const QByteArray& bytes)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }
    file.write(bytes);
    return true;
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir temp;
    if (!temp.isValid())
    {
        QTextStream(stderr) << "temporary directory is invalid\n";
        return 1;
    }

    const QString workspace = temp.path();
    const QString testdiffSummary = QDir(workspace).filePath(QStringLiteral("verification/testdiff_summary.txt"));
    const QString diffImage = QDir(workspace).filePath(QStringLiteral("artifacts/testdiff/diff/view.png"));
    if (!writeFile(testdiffSummary, "view diff 2px changed\n")
        || !writeFile(diffImage, "png"))
    {
        QTextStream(stderr) << "failed to create fixture files\n";
        return 2;
    }

    occtdebug::CommandResult gate;
    gate.program = QStringLiteral("ctest");
    gate.arguments = {QStringLiteral("-R"), QStringLiteral("draw_smoke")};
    gate.stdoutText = QStringLiteral("DRAW_SMOKE_OK\n");
    gate.exitCode = 0;
    gate.exitStatus = QProcess::NormalExit;
    gate.elapsedMs = 25;

    occtdebug::CommandResult command;
    command.program = QStringLiteral("testgrid");
    command.stdoutText = QStringLiteral(
        "module run pass fail pass_rate\n"
        "Modeling 2 1 1 50%\n"
        "view diff 2px changed\n");
    command.exitCode = 1;
    command.exitStatus = QProcess::NormalExit;
    command.elapsedMs = 125;

    occtdebug::TwoStagePhaseResultWriterResult result;
    QString error;
    if (!occtdebug::TwoStagePhaseResultWriter::writePhaseResult({
            QStringLiteral("CASE-PHASE-SMOKE"),
            workspace,
            QStringLiteral("after"),
            QStringLiteral("after phase finished"),
            gate,
            true,
            command,
        },
        &result,
        &error))
    {
        QTextStream(stderr) << "writer failed: " << error << "\n";
        return 3;
    }

    QFile file(result.artifactPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream(stderr) << "failed to read phase artifact\n";
        return 4;
    }
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    const QJsonObject artifacts = root.value(QStringLiteral("testdiff_artifacts")).toObject();
    const QJsonObject artifactIndex = artifacts.value(QStringLiteral("artifact_index")).toObject();

    if (result.status != QStringLiteral("written"))
    {
        QTextStream(stderr) << "status mismatch: status=" << result.status
                            << " gate_exit_code=" << gate.exitCode
                            << " gate_canceled=" << gate.canceled
                            << " gate_timed_out=" << gate.timedOut
                            << "\n";
        return 5;
    }

    if (!expect(result.rows.size() == 2, "row count mismatch")
        || !expect(result.testdiff.changedCount == 1, "testdiff changed count mismatch")
        || !expect(!result.failureDetails.isEmpty(), "failure details are empty")
        || !expect(result.timing.entries.size() == 2, "timing entry count mismatch")
        || !expect(result.artifactRelativePath == QStringLiteral("artifacts/testgrid_after_result.json"), "artifact relative path mismatch")
        || !expect(QFile::exists(QDir(workspace).filePath(QStringLiteral("logs/testgrid_after_gate.stdout.log"))), "gate stdout log missing")
        || !expect(QFile::exists(QDir(workspace).filePath(QStringLiteral("logs/testgrid_after.stdout.log"))), "command stdout log missing")
        || !expect(QFile::exists(QDir(workspace).filePath(QStringLiteral("verification/testgrid_after.txt"))), "phase summary missing")
        || !expect(root.value(QStringLiteral("phase")).toString() == QStringLiteral("after"), "phase json mismatch")
        || !expect(root.value(QStringLiteral("testgrid_rows")).toArray().size() == 2, "phase rows json mismatch")
        || !expect(artifactIndex.value(QStringLiteral("groups")).toArray().size() == 1, "artifact index group missing"))
    {
        return 5;
    }

    QTextStream(stdout) << "TWO_STAGE_PHASE_RESULT_WRITER_SMOKE_OK\n";
    return 0;
}
