#include "core/verify/TestdiffArtifactAnalysis.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
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
    QTemporaryDir workspace;
    if (!expect(workspace.isValid(), "workspace temporary directory is invalid"))
    {
        return 1;
    }

    const QDir root(workspace.path());
    if (!expect(writeFile(root.filePath(QStringLiteral("artifacts/testdiff/diff/view.png")), "png"), "image fixture failed")
        || !expect(writeFile(root.filePath(QStringLiteral("artifacts/testdiff/diff/props.json")), "{\"changed_edges\":1,\"faces\":2}"), "property fixture failed")
        || !expect(writeFile(root.filePath(QStringLiteral("artifacts/testdiff/diff/performance_timing.txt")), "fillet_case +0.8%\n42ms\n"), "performance fixture failed"))
    {
        return 2;
    }

    const QJsonObject artifactIndex {
        {QStringLiteral("groups"), QJsonArray {
             QJsonObject {
                 {QStringLiteral("kind"), QStringLiteral("image")},
                 {QStringLiteral("key"), QStringLiteral("view")},
                 {QStringLiteral("status"), QStringLiteral("diff_only")},
                 {QStringLiteral("diff"), QStringLiteral("artifacts/testdiff/diff/view.png")},
             },
             QJsonObject {
                 {QStringLiteral("kind"), QStringLiteral("property")},
                 {QStringLiteral("key"), QStringLiteral("props")},
                 {QStringLiteral("status"), QStringLiteral("diff_only")},
                 {QStringLiteral("diff"), QStringLiteral("artifacts/testdiff/diff/props.json")},
             },
             QJsonObject {
                 {QStringLiteral("kind"), QStringLiteral("performance")},
                 {QStringLiteral("key"), QStringLiteral("performance_timing")},
                 {QStringLiteral("status"), QStringLiteral("diff_only")},
                 {QStringLiteral("diff"), QStringLiteral("artifacts/testdiff/diff/performance_timing.txt")},
             },
         }},
    };

    const QJsonObject analysis = occtdebug::TestdiffArtifactAnalysis::build(workspace.path(), artifactIndex);
    const QJsonObject summary = analysis.value(QStringLiteral("summary")).toObject();
    const QJsonArray groups = analysis.value(QStringLiteral("groups")).toArray();
    const QJsonObject generationPolicy = analysis.value(QStringLiteral("generation_policy")).toObject();
    const QJsonObject generationContract = generationPolicy.value(QStringLiteral("contract")).toObject();
    const QJsonArray generators = generationPolicy.value(QStringLiteral("generators")).toArray();

    QJsonObject propertyJson;
    QJsonArray performanceMetrics;
    for (const QJsonValue& value : groups)
    {
        const QJsonObject group = value.toObject();
        if (group.value(QStringLiteral("kind")).toString() == QStringLiteral("property"))
        {
            propertyJson = group.value(QStringLiteral("analysis")).toObject().value(QStringLiteral("json")).toObject();
        }
        if (group.value(QStringLiteral("kind")).toString() == QStringLiteral("performance"))
        {
            performanceMetrics = group.value(QStringLiteral("analysis")).toObject().value(QStringLiteral("metrics")).toArray();
        }
    }

    if (!expect(summary.value(QStringLiteral("groups")).toInt() == 3, "group summary mismatch")
        || !expect(summary.value(QStringLiteral("image_diff_supplied_by_runner")).toInt() == 1, "image diff summary mismatch")
        || !expect(summary.value(QStringLiteral("property_json_parsed")).toInt() == 1, "property parsed summary mismatch")
        || !expect(summary.value(QStringLiteral("performance_metrics")).toInt() == 2, "performance metric summary mismatch")
        || !expect(propertyJson.value(QStringLiteral("top_level_key_count")).toInt() == 2, "json key count mismatch")
        || !expect(performanceMetrics.size() == 2, "performance metric count mismatch")
        || !expect(!analysis.value(QStringLiteral("limits")).toObject().value(QStringLiteral("pixel_diff_generated")).toBool(), "pixel diff limit mismatch")
        || !expect(generationPolicy.value(QStringLiteral("policy")).toString() == QStringLiteral("boundary_only"), "generation policy mode mismatch")
        || !expect(!generationPolicy.value(QStringLiteral("generation_performed")).toBool(), "generation policy performed mismatch")
        || !expect(generationContract.value(QStringLiteral("mode")).toString() == QStringLiteral("opt_in"), "generation contract mode mismatch")
        || !expect(generationContract.value(QStringLiteral("output_root")).toString() == QStringLiteral("artifacts/testdiff/generated"), "generation contract output root mismatch")
        || !expect(generators.size() == 3, "generation policy generator count mismatch"))
    {
        return 3;
    }

    QTextStream(stdout) << "TESTDIFF_ARTIFACT_ANALYSIS_SMOKE_OK\n";
    return 0;
}
