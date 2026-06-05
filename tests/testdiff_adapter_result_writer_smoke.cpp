#include "core/verify/TestdiffAdapterResultWriter.h"

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

QJsonObject readJson(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return {};
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    return document.isObject() ? document.object() : QJsonObject();
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir workspace;
    QTemporaryDir output;
    if (!expect(workspace.isValid(), "workspace temporary directory is invalid")
        || !expect(output.isValid(), "output temporary directory is invalid"))
    {
        return 1;
    }

    QDir outputDir(output.path());
    if (!expect(outputDir.mkpath(QStringLiteral("before")), "failed to create before directory")
        || !expect(outputDir.mkpath(QStringLiteral("after")), "failed to create after directory")
        || !expect(outputDir.mkpath(QStringLiteral("diff")), "failed to create diff directory")
        || !expect(writeFile(outputDir.filePath(QStringLiteral("before/view.png")), QByteArray("before")), "failed to write before image")
        || !expect(writeFile(outputDir.filePath(QStringLiteral("after/view.png")), QByteArray("after")), "failed to write after image")
        || !expect(writeFile(outputDir.filePath(QStringLiteral("diff/view.png")), QByteArray("diff")), "failed to write diff image"))
    {
        return 2;
    }

    occtdebug::CommandResult command;
    command.program = QStringLiteral("testdiff-runner");
    command.workingDirectory = workspace.path();
    command.exitStatus = QProcess::NormalExit;
    command.exitCode = 0;
    command.elapsedMs = 17;
    command.stdoutText = QStringLiteral("view: changed pixels 3\n");

    occtdebug::VerificationPlan plan;
    plan.testdiffExecutable = QStringLiteral("testdiff-runner");
    plan.testdiffArguments = QStringLiteral("--out {output}");

    occtdebug::TestdiffAdapterResultWriterResult result;
    QString error;
    if (!expect(occtdebug::TestdiffAdapterResultWriter::writeResult({
                    workspace.path(),
                    plan,
                    output.path(),
                    {
                        {QStringLiteral("draw smoke"), QStringLiteral("1"), QStringLiteral("1"), QStringLiteral("0"), QStringLiteral("100%")},
                    },
                    command,
                    QStringLiteral("writer smoke"),
                },
                &result,
                &error),
                "writer failed"))
    {
        QTextStream(stderr) << error << "\n";
        return 3;
    }

    const QJsonObject adapter = readJson(result.adapterResultPath);
    const QString outputRoot = adapter.value(QStringLiteral("output_root")).toString();
    const QJsonObject testgrid = readJson(result.testgridResultPath);
    const QJsonObject artifacts = testgrid.value(QStringLiteral("testdiff_artifacts")).toObject();
    const QJsonObject artifactIndex = artifacts.value(QStringLiteral("artifact_index")).toObject();
    const QJsonObject artifactAnalysis = artifacts.value(QStringLiteral("artifact_analysis")).toObject();
    if (result.status != QStringLiteral("passed"))
    {
        QTextStream(stderr) << "writer status mismatch: status=" << result.status
                            << " import_success=" << result.importResult.success
                            << " copied=" << result.importResult.copiedFiles
                            << " import_error=" << result.importResult.error
                            << " exit_code=" << command.exitCode
                            << " canceled=" << command.canceled
                            << " timed_out=" << command.timedOut
                            << "\n";
        return 4;
    }

    if (!expect(result.importResult.copiedFiles == 3, "copied files mismatch")
        || !expect(!outputRoot.contains(output.path()) && !outputRoot.contains(QLatin1Char(':')), "output root should not leak absolute path")
        || !expect(testgrid.value(QStringLiteral("testdiff_adapter_status")).toString() == QStringLiteral("passed"), "testgrid adapter status mismatch")
        || !expect(artifactIndex.value(QStringLiteral("groups")).toArray().size() == 1, "artifact index group count mismatch")
        || !expect(artifactAnalysis.value(QStringLiteral("summary")).toObject().value(QStringLiteral("image_diff_supplied_by_runner")).toInt() == 1, "artifact analysis image summary mismatch")
        || !expect(artifactAnalysis.value(QStringLiteral("generation_policy")).toObject().value(QStringLiteral("policy")).toString() == QStringLiteral("boundary_only"), "generation policy mode mismatch")
        || !expect(QFile::exists(QDir(workspace.path()).filePath(QStringLiteral("logs/testdiff_runner.stdout.log"))), "stdout log missing")
        || !expect(QFile::exists(QDir(workspace.path()).filePath(QStringLiteral("verification/testdiff_summary.txt"))), "summary missing"))
    {
        return 4;
    }

    QTextStream(stdout) << "TESTDIFF_ADAPTER_RESULT_WRITER_SMOKE_OK\n";
    return 0;
}
