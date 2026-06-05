#include "core/verify/TwoStageVerificationResultWriter.h"
#include "core/verify/TwoStageFinalResultWriter.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QTemporaryDir>

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
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }
    file.write(bytes);
    return true;
}

QJsonObject readJson(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        return {};
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    return document.isObject() ? document.object() : QJsonObject {};
}

occtdebug::CommandResult commandResult(const QString& program, int exitCode, qint64 elapsedMs)
{
    occtdebug::CommandResult result;
    result.program = program;
    result.arguments = {QStringLiteral("--demo")};
    result.workingDirectory = QStringLiteral("<workspace>");
    result.exitCode = exitCode;
    result.exitStatus = QProcess::NormalExit;
    result.elapsedMs = elapsedMs;
    return result;
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir workspace;
    if (!expect(workspace.isValid(), "temporary workspace is invalid"))
    {
        return 1;
    }
    QDir root(workspace.path());
    if (!expect(root.mkpath(QStringLiteral("verification/testdiff/diff")), "failed to create testdiff diff directory")
        || !expect(root.mkpath(QStringLiteral("verification/testdiff/before")), "failed to create testdiff before directory")
        || !expect(writeFile(root.filePath(QStringLiteral("verification/testdiff/diff/geometry.png")), QByteArray("png")),
                   "failed to write image artifact")
        || !expect(writeFile(root.filePath(QStringLiteral("verification/testdiff/diff/properties.json")), QByteArray("{}")),
                   "failed to write property artifact")
        || !expect(writeFile(root.filePath(QStringLiteral("verification/testdiff/before/baseline.log")), QByteArray("baseline")),
                   "failed to write before log artifact"))
    {
        return 4;
    }

    QVector<occtdebug::TestgridRow> rows {
        {QStringLiteral("bugs mod demo"), QStringLiteral("2"), QStringLiteral("1"), QStringLiteral("1"), QStringLiteral("50%")},
    };
    occtdebug::TestdiffSummary testdiff;
    testdiff.entries.push_back({QStringLiteral("bugs/demo"), QStringLiteral("changed"), QStringLiteral("pixels"), QStringLiteral("review")});
    testdiff.changedCount = 1;
    QVector<occtdebug::VerificationFailureDetail> failures {
        {QStringLiteral("testgrid"), QStringLiteral("bugs mod demo"), QStringLiteral("failed"), QStringLiteral("1 failed out of 2"), QStringLiteral("artifacts/testgrid_after_result.json")},
    };
    occtdebug::VerificationTimingSummary timing =
        occtdebug::VerificationResultParser::timingSummary({
            {QStringLiteral("before_draw_smoke_gate"), 10, QStringLiteral("passed")},
            {QStringLiteral("after_draw_smoke_gate"), 20, QStringLiteral("passed")},
        });

    const QJsonObject phase = occtdebug::TwoStageVerificationResultWriter::buildPhaseResult({
        QStringLiteral("OCC-LOCAL-WRITER"),
        QStringLiteral("after"),
        QStringLiteral("after phase completed"),
        workspace.path(),
        commandResult(QStringLiteral("ctest"), 0, 10),
        true,
        QStringLiteral("logs/testgrid_after_gate.stdout.log"),
        QStringLiteral("logs/testgrid_after_gate.stderr.log"),
        true,
        commandResult(QStringLiteral("testgrid"), 1, 40),
        QStringLiteral("logs/testgrid_after.stdout.log"),
        QStringLiteral("logs/testgrid_after.stderr.log"),
        QString(),
        QString(),
        rows,
        testdiff,
        failures,
        timing,
    });

    if (!expect(phase.value(QStringLiteral("schema_version")).toInt() == 1, "phase schema mismatch")
        || !expect(phase.value(QStringLiteral("phase")).toString() == QStringLiteral("after"), "phase name mismatch")
        || !expect(phase.value(QStringLiteral("testgrid_rows")).toArray().size() == 1, "phase rows missing")
        || !expect(phase.value(QStringLiteral("failure_details")).toArray().size() == 1, "phase failures missing")
        || !expect(phase.value(QStringLiteral("timing")).toObject().value(QStringLiteral("total_elapsed_ms")).toDouble() == 30.0, "phase timing mismatch")
        || !expect(phase.value(QStringLiteral("testdiff_artifacts")).toObject().value(QStringLiteral("command_stdout")).toString().startsWith(QStringLiteral("logs/")), "phase command stdout missing")
        || !expect(phase.value(QStringLiteral("testdiff_artifacts")).toObject().value(QStringLiteral("artifact_files")).toArray().size() == 3, "phase artifact files missing")
        || !expect(phase.value(QStringLiteral("testdiff_artifacts")).toObject().value(QStringLiteral("artifact_counts")).toObject().value(QStringLiteral("image")).toInt() == 1, "phase image artifact count mismatch"))
    {
        return 2;
    }

    const occtdebug::TestgridComparison comparison =
        occtdebug::VerificationResultParser::compareTestgridRows({}, rows);
    occtdebug::VerificationPlan plan;
    plan.testgridExecutable = QStringLiteral("testgrid.exe");
    const QJsonObject finalResult = occtdebug::TwoStageVerificationResultWriter::buildWorkflowResult({
        QStringLiteral("OCC-LOCAL-WRITER"),
        QStringLiteral("completed"),
        QStringLiteral("done"),
        workspace.path(),
        plan,
        false,
        rows,
        testdiff,
        failures,
        timing,
        QJsonObject {{QStringLiteral("two_stage_result"), QStringLiteral("artifacts/testgrid_two_stage_result.json")}},
        comparison,
        QString(),
        QString(),
    });

    if (!expect(finalResult.value(QStringLiteral("mode")).toString() == QStringLiteral("two_stage"), "final mode mismatch")
        || !expect(finalResult.value(QStringLiteral("testgrid_plan")).toObject().value(QStringLiteral("testgrid_executable")).toString() == QStringLiteral("testgrid.exe"), "final plan mismatch")
        || !expect(finalResult.value(QStringLiteral("phases")).toObject().value(QStringLiteral("before")).toString().contains(QStringLiteral("before_result")), "final phase artifact missing")
        || !expect(finalResult.value(QStringLiteral("before_after")).toObject().contains(QStringLiteral("status")), "final before_after missing")
        || !expect(finalResult.value(QStringLiteral("testdiff_artifacts")).toObject().value(QStringLiteral("two_stage_result")).toString().contains(QStringLiteral("two_stage")), "final testdiff artifact missing"))
    {
        return 3;
    }

    occtdebug::TwoStageFinalResultWriterResult writeResult;
    QString writeError;
    if (!expect(occtdebug::TwoStageFinalResultWriter::writeFinalResult({
            QStringLiteral("OCC-LOCAL-WRITER"),
            workspace.path(),
            QStringLiteral("completed"),
            QStringLiteral("done"),
            plan,
            false,
            rows,
            testdiff,
            failures,
            timing,
            QJsonObject {{QStringLiteral("two_stage_result"), QStringLiteral("artifacts/testgrid_two_stage_result.json")}},
            comparison,
            QString(),
            QString(),
        },
        &writeResult,
        &writeError),
        "final result writer failed")
        || !expect(QFile::exists(writeResult.twoStageResultPath), "two-stage final artifact was not written")
        || !expect(QFile::exists(writeResult.legacyResultPath), "legacy final artifact was not written")
        || !expect(readJson(writeResult.legacyResultPath).value(QStringLiteral("mode")).toString() == QStringLiteral("two_stage"),
                   "legacy final artifact content mismatch"))
    {
        if (!writeError.isEmpty())
        {
            QTextStream(stderr) << writeError << "\n";
        }
        return 5;
    }

    QTextStream(stdout) << "TWO_STAGE_VERIFICATION_RESULT_WRITER_SMOKE_OK\n";
    return 0;
}
