#include "core/verify/TestgridArtifactService.h"
#include "core/verify/TestgridResultWriter.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include <iostream>

namespace
{
bool expect(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        return false;
    }
    return true;
}

QJsonObject readJsonObject(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return {};
    }
    return QJsonDocument::fromJson(file.readAll()).object();
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir temp;
    if (!expect(temp.isValid(), "temporary workspace creation failed"))
    {
        return 1;
    }

    QString error;
    if (!expect(occtdebug::TestgridArtifactService::ensureWorkspaceDirectories(temp.path(), &error),
                "workspace directory creation failed"))
    {
        std::cerr << error.toStdString() << '\n';
        return 1;
    }

    const QString beforePath = occtdebug::TestgridArtifactService::verificationPath(temp.path(), QStringLiteral("testgrid_before.txt"));
    if (!expect(occtdebug::TestgridArtifactService::writeTextArtifact(
                    beforePath,
                    QStringLiteral("Modeling 2 2 0 100%\n"),
                    &error),
                "before summary write failed"))
    {
        std::cerr << error.toStdString() << '\n';
        return 1;
    }

    occtdebug::VerificationPlan plan;
    plan.testgridGroup = QStringLiteral("bugs");
    plan.testgridGrid = QStringLiteral("moddata");
    plan.testgridCase = QStringLiteral("bug123");

    occtdebug::CommandResult gate;
    gate.program = QStringLiteral("ctest");
    gate.arguments = {QStringLiteral("-R"), QStringLiteral("draw_smoke")};
    gate.exitCode = 0;
    gate.elapsedMs = 10;

    occtdebug::CommandResult command;
    command.program = QStringLiteral("testgrid");
    command.arguments = {plan.testgridGroup, plan.testgridGrid, plan.testgridCase};
    command.stdoutText = QStringLiteral("Modeling 2 1 1 50%\ncaseA failed tol-diff\n");
    command.exitCode = 1;
    command.elapsedMs = 25;

    occtdebug::TestgridResultWriterResult result;
    const occtdebug::TestgridResultWriterInput input {
        QStringLiteral("OCC-SMOKE"),
        temp.path(),
        QStringLiteral("writer smoke"),
        plan,
        gate,
        true,
        command,
    };
    if (!expect(occtdebug::TestgridResultWriter::writeSingleStageResult(input, &result, &error),
                "single-stage testgrid result write failed"))
    {
        std::cerr << error.toStdString() << '\n';
        return 1;
    }

    const QJsonObject json = readJsonObject(result.artifactPath);
    const QString serialized = QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact));

    bool ok = true;
    if (!result.gatePassed)
    {
        std::cerr << "gate should pass: exit_code=" << gate.exitCode
                  << " canceled=" << gate.canceled
                  << " timed_out=" << gate.timedOut
                  << '\n';
        ok = false;
    }
    ok = expect(result.commandExecuted, "command should be marked executed") && ok;
    ok = expect(result.rows.size() == 1, "parsed testgrid rows mismatch") && ok;
    ok = expect(result.failureDetails.size() >= 1, "failure details should include failed row") && ok;
    ok = expect(result.timing.totalElapsedMs == 35, "timing summary mismatch") && ok;
    ok = expect(!result.verificationItems.isEmpty(), "verification items should be generated") && ok;
    ok = expect(result.diffSummary.contains(QStringLiteral("delta_fail=1")), "before/after summary mismatch") && ok;
    ok = expect(json.value(QStringLiteral("testgrid_rows")).toArray().size() == 1, "json rows mismatch") && ok;
    ok = expect(json.value(QStringLiteral("testgrid_plan")).toObject().value(QStringLiteral("testgrid_group")).toString() == QStringLiteral("bugs"),
                "plan json mismatch") && ok;
    ok = expect(serialized.contains(QStringLiteral("verification/testgrid_before.txt")), "relative before summary missing") && ok;
    ok = expect(!serialized.contains(temp.path()), "workspace absolute path leaked into result json") && ok;

    if (ok)
    {
        std::cout << "TESTGRID_RESULT_WRITER_SMOKE_OK\n";
    }
    return ok ? 0 : 1;
}
