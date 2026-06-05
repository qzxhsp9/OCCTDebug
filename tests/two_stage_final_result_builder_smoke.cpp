#include "core/verify/TestgridArtifactService.h"
#include "core/verify/TwoStageFinalResultBuilder.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
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
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }
    file.write(bytes);
    return true;
}

occtdebug::CommandResult commandResult(const QString& program,
                                       int exitCode,
                                       qint64 elapsedMs,
                                       const QString& stdoutText = {},
                                       const QString& stderrText = {})
{
    occtdebug::CommandResult result;
    result.program = program;
    result.arguments = {QStringLiteral("--smoke")};
    result.workingDirectory = QStringLiteral("<workspace>");
    result.exitCode = exitCode;
    result.exitStatus = QProcess::NormalExit;
    result.elapsedMs = elapsedMs;
    result.stdoutText = stdoutText;
    result.stderrText = stderrText;
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

    QString error;
    if (!expect(occtdebug::TestgridArtifactService::ensureWorkspaceDirectories(workspace.path(), &error),
                "workspace directories failed"))
    {
        QTextStream(stderr) << error << "\n";
        return 2;
    }

    const QVector<occtdebug::TestgridRow> beforeRows {
        {QStringLiteral("Modeling"), QStringLiteral("2"), QStringLiteral("2"), QStringLiteral("0"), QStringLiteral("100%")},
    };
    const QVector<occtdebug::TestgridRow> afterRows {
        {QStringLiteral("Modeling"), QStringLiteral("2"), QStringLiteral("1"), QStringLiteral("1"), QStringLiteral("50%")},
    };
    QString beforeSummaryPath;
    QString afterSummaryPath;
    if (!expect(occtdebug::TestgridArtifactService::writePhaseSummary(workspace.path(), QStringLiteral("before"), beforeRows, &beforeSummaryPath, &error),
                "before summary write failed")
        || !expect(occtdebug::TestgridArtifactService::writePhaseSummary(workspace.path(), QStringLiteral("after"), afterRows, &afterSummaryPath, &error),
                   "after summary write failed"))
    {
        QTextStream(stderr) << error << "\n";
        return 3;
    }

    QDir root(workspace.path());
    if (!expect(root.mkpath(QStringLiteral("verification/testdiff/diff")), "testdiff dir creation failed")
        || !expect(writeFile(root.filePath(QStringLiteral("verification/testdiff/diff/view.png")), QByteArray("png")),
                   "image artifact write failed")
        || !expect(occtdebug::TestgridArtifactService::writeTextArtifact(
                occtdebug::TestgridArtifactService::verificationPath(workspace.path(), QStringLiteral("testdiff_summary.txt")),
                QStringLiteral("view changed 2px\n"),
                &error),
            "testdiff summary write failed"))
    {
        QTextStream(stderr) << error << "\n";
        return 4;
    }

    occtdebug::VerificationPlan plan;
    plan.testgridGroup = QStringLiteral("bugs");
    plan.testgridGrid = QStringLiteral("modalg");
    plan.testgridCase = QStringLiteral("bug_builder");

    const occtdebug::TwoStageFinalResultBuilderResult result =
        occtdebug::TwoStageFinalResultBuilder::build({
            QStringLiteral("OCC-BUILDER"),
            workspace.path(),
            QStringLiteral("failed"),
            QStringLiteral("after regression"),
            plan,
            true,
            true,
            true,
            commandResult(QStringLiteral("ctest"), 0, 10),
            commandResult(QStringLiteral("testgrid"), 0, 20),
            commandResult(QStringLiteral("ctest"), 0, 30),
            commandResult(QStringLiteral("testgrid"), 1, 40, QStringLiteral("runner_diff changed pixels\n")),
        });

    const QJsonObject artifacts = result.writerInput.testdiffArtifacts;
    if (!expect(result.writerInput.caseId == QStringLiteral("OCC-BUILDER"), "writer case id mismatch")
        || !expect(result.writerInput.patchApplied, "patch applied mismatch")
        || !expect(result.afterRows.size() == 1, "after rows missing")
        || !expect(result.comparison.hasRegression(), "comparison regression mismatch")
        || !expect(result.testdiff.entries.size() == 1, "testdiff command parsing mismatch")
        || !expect(!result.failureDetails.isEmpty(), "failure details missing")
        || !expect(result.timing.totalElapsedMs == 100, "timing total mismatch")
        || !expect(result.writerInput.beforeSummaryPath == beforeSummaryPath, "before summary path mismatch")
        || !expect(result.writerInput.afterSummaryPath == afterSummaryPath, "after summary path mismatch")
        || !expect(artifacts.value(QStringLiteral("two_stage_result")).toString() == QStringLiteral("artifacts/testgrid_two_stage_result.json"),
                   "two stage artifact pointer missing")
        || !expect(artifacts.value(QStringLiteral("artifact_files")).toArray().size() >= 1, "testdiff artifact scan mismatch")
        || !expect(!QString::fromUtf8(QJsonDocument(artifacts).toJson(QJsonDocument::Compact)).contains(workspace.path()),
                   "workspace absolute path leaked"))
    {
        return 5;
    }

    QTextStream(stdout) << "TWO_STAGE_FINAL_RESULT_BUILDER_SMOKE_OK\n";
    return 0;
}
