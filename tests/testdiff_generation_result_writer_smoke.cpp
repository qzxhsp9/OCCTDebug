#include "core/verify/TestdiffGenerationPolicy.h"
#include "core/verify/TestdiffGenerationResultWriter.h"

#include <QCoreApplication>
#include <QFile>
#include <QIODevice>
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

QJsonObject readJson(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return {};
    }
    return QJsonDocument::fromJson(file.readAll()).object();
}

bool containsNoDrivePath(const QString& text)
{
    return !text.contains(QStringLiteral(":/"))
        && !text.contains(QStringLiteral(":\\"));
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    QTemporaryDir workspace;
    if (!workspace.isValid())
    {
        QTextStream(stderr) << "temporary workspace is invalid\n";
        return 1;
    }

    const QJsonObject artifactIndex {
        {QStringLiteral("groups"), QJsonArray {
             QJsonObject {
                 {QStringLiteral("kind"), QStringLiteral("image")},
                 {QStringLiteral("key"), QStringLiteral("view")},
                 {QStringLiteral("before"), QStringLiteral("artifacts/testdiff/before/view.png")},
                 {QStringLiteral("after"), QStringLiteral("artifacts/testdiff/after/view.png")},
             },
         }},
    };
    const QJsonObject generationConfig {
        {QStringLiteral("enabled_generators"), QJsonArray {QStringLiteral("image_pixel_diff")}},
        {QStringLiteral("failure_report"), QJsonObject {
             {QStringLiteral("path"), QStringLiteral("artifacts/testdiff/generated/failure_report.json")},
         }},
    };
    const QJsonObject policy = occtdebug::TestdiffGenerationPolicy::build(
        artifactIndex,
        QJsonObject {},
        generationConfig);

    QString error;
    const QString reportPath = QStringLiteral("artifacts/testdiff/generated/failure_report.json");
    if (!occtdebug::TestdiffGenerationResultWriter::writeFailureReport(
            workspace.path(),
            QStringLiteral("OCC-LOCAL-SMOKE"),
            policy,
            reportPath,
            &error))
    {
        QTextStream(stderr) << "failed to write failure report: " << error << "\n";
        return 1;
    }

    const QString artifactPath = QStringLiteral("artifacts/testdiff/generated/image/view.pixel_diff.png");
    if (!occtdebug::TestdiffGenerationResultWriter::writeSidecar(
            workspace.path(),
            {
                QStringLiteral("image_pixel_diff"),
                QStringLiteral("image"),
                QStringLiteral("diff"),
                artifactPath,
                QStringLiteral("not_generated"),
                QJsonArray {
                    QStringLiteral("artifacts/testdiff/before/view.png"),
                    QStringLiteral("artifacts/testdiff/after/view.png"),
                    QStringLiteral("private:runner-output/view.png"),
                },
                QJsonObject {{QStringLiteral("pixel_abs"), 2.0}},
                QStringLiteral("future-image-pixel-diff"),
                QStringLiteral("blocked"),
                QStringLiteral("sidecar contract smoke"),
            },
            &error))
    {
        QTextStream(stderr) << "failed to write sidecar: " << error << "\n";
        return 1;
    }

    const QJsonObject report = readJson(workspace.filePath(reportPath));
    const QJsonArray issues = report.value(QStringLiteral("issues")).toArray();
    const QString sidecarPath = occtdebug::TestdiffGenerationResultWriter::sidecarPathForArtifact(artifactPath);
    const QJsonObject sidecar = readJson(workspace.filePath(sidecarPath));
    const QString sidecarText = QString::fromUtf8(QJsonDocument(sidecar).toJson(QJsonDocument::Compact));

    if (!expect(report.value(QStringLiteral("case_id")).toString() == QStringLiteral("OCC-LOCAL-SMOKE"), "case id mismatch")
        || !expect(report.value(QStringLiteral("status")).toString() == QStringLiteral("blocked"), "failure report status mismatch")
        || !expect(issues.size() == 1, "failure report issue count mismatch")
        || !expect(issues.first().toObject().value(QStringLiteral("generator_id")).toString() == QStringLiteral("image_pixel_diff"), "issue generator mismatch")
        || !expect(report.value(QStringLiteral("privacy")).toObject().value(QStringLiteral("case_relative_paths_only")).toBool(), "privacy flag missing")
        || !expect(sidecar.value(QStringLiteral("artifact")).toString() == artifactPath, "sidecar artifact mismatch")
        || !expect(sidecar.value(QStringLiteral("artifact_status")).toString() == QStringLiteral("not_generated"), "sidecar artifact status mismatch")
        || !expect(sidecar.value(QStringLiteral("input_artifacts")).toArray().size() == 2, "sidecar should drop non-relative input paths")
        || !expect(containsNoDrivePath(sidecarText), "sidecar leaked drive path"))
    {
        return 1;
    }

    QTextStream(stdout) << "TESTDIFF_GENERATION_RESULT_WRITER_SMOKE_OK\n";
    return 0;
}
