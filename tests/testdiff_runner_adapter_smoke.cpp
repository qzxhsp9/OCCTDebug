#include "core/verify/TestdiffRunnerAdapter.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
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
    QTemporaryDir workspace;
    QTemporaryDir runnerOutput;
    if (!expect(workspace.isValid(), "workspace temporary directory is invalid")
        || !expect(runnerOutput.isValid(), "runner output temporary directory is invalid"))
    {
        return 1;
    }

    QDir source(runnerOutput.path());
    if (!expect(source.mkpath(QStringLiteral("before")), "failed to create before output")
        || !expect(source.mkpath(QStringLiteral("after")), "failed to create after output")
        || !expect(source.mkpath(QStringLiteral("diff")), "failed to create diff output")
        || !expect(writeFile(source.filePath(QStringLiteral("before/baseline.log")), QByteArray("baseline")), "failed to write before log")
        || !expect(writeFile(source.filePath(QStringLiteral("after/patched.log")), QByteArray("patched")), "failed to write after log")
        || !expect(writeFile(source.filePath(QStringLiteral("diff/image.png")), QByteArray("png")), "failed to write image diff")
        || !expect(writeFile(source.filePath(QStringLiteral("diff/properties.json")), QByteArray("{}")), "failed to write property diff")
        || !expect(writeFile(source.filePath(QStringLiteral("diff/performance_timing.txt")), QByteArray("42ms")), "failed to write timing diff"))
    {
        return 2;
    }

    QDir workspaceDir(workspace.path());
    if (!expect(workspaceDir.mkpath(QStringLiteral("verification")), "failed to create verification directory")
        || !expect(writeFile(workspaceDir.filePath(QStringLiteral("verification/testdiff_summary.txt")),
                            QByteArray("geometry diff pixels 1 review\n")), "failed to write summary"))
    {
        return 3;
    }

    const occtdebug::TestdiffRunnerImportResult result = occtdebug::TestdiffRunnerAdapter::importOutput(
        workspace.path(),
        runnerOutput.path(),
        workspaceDir.filePath(QStringLiteral("verification/testdiff_summary.txt")),
        QStringLiteral("logs/testgrid.stdout.log"),
        QStringLiteral("logs/testgrid.stderr.log"),
        3,
        2,
        1);

    if (!expect(result.success, "adapter import failed"))
    {
        QTextStream(stderr) << result.error << "\n";
        return 4;
    }
    const QJsonObject counts = result.manifest.value(QStringLiteral("artifact_counts")).toObject();
    const QJsonObject artifactIndex = result.manifest.value(QStringLiteral("artifact_index")).toObject();
    const QJsonObject artifactAnalysis = result.manifest.value(QStringLiteral("artifact_analysis")).toObject();
    const QJsonObject indexCounts = artifactIndex.value(QStringLiteral("counts")).toObject();
    const QJsonObject indexImageCounts = indexCounts.value(QStringLiteral("image")).toObject();
    if (!expect(result.copiedFiles == 5, "copied file count mismatch")
        || !expect(result.manifest.value(QStringLiteral("artifact_files")).toArray().size() == 5, "artifact file manifest mismatch")
        || !expect(counts.value(QStringLiteral("image")).toInt() == 1, "image count mismatch")
        || !expect(counts.value(QStringLiteral("property")).toInt() == 1, "property count mismatch")
        || !expect(counts.value(QStringLiteral("performance")).toInt() == 1, "performance count mismatch")
        || !expect(artifactIndex.value(QStringLiteral("groups")).toArray().size() == 3, "artifact index group count mismatch")
        || !expect(indexImageCounts.value(QStringLiteral("diff")).toInt() == 1, "artifact index image diff count mismatch")
        || !expect(artifactAnalysis.value(QStringLiteral("summary")).toObject().value(QStringLiteral("performance_metrics")).toInt() == 1, "artifact analysis performance metric mismatch")
        || !expect(artifactAnalysis.value(QStringLiteral("generation_policy")).toObject().value(QStringLiteral("policy")).toString() == QStringLiteral("boundary_only"), "generation policy mode mismatch")
        || !expect(result.manifest.value(QStringLiteral("adapter")).toObject().value(QStringLiteral("copied_files")).toInt() == 5, "adapter metadata mismatch"))
    {
        return 5;
    }

    const QString manifestText = QString::fromUtf8(QJsonDocument(result.manifest).toJson(QJsonDocument::Compact));
    if (!expect(!manifestText.contains(runnerOutput.path()), "manifest leaked runner absolute path"))
    {
        return 6;
    }

    QTextStream(stdout) << "TESTDIFF_RUNNER_ADAPTER_SMOKE_OK\n";
    return 0;
}
