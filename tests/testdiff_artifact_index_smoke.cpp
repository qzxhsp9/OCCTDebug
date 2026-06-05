#include "core/verify/TestdiffArtifactIndex.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
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

QJsonObject fileEntry(const QString& role, const QString& kind, const QString& path, int bytes)
{
    return {
        {QStringLiteral("role"), role},
        {QStringLiteral("kind"), kind},
        {QStringLiteral("path"), path},
        {QStringLiteral("bytes"), bytes},
    };
}

QJsonObject findGroup(const QJsonArray& groups, const QString& kind, const QString& key)
{
    for (const QJsonValue& value : groups)
    {
        const QJsonObject group = value.toObject();
        if (group.value(QStringLiteral("kind")).toString() == kind
            && group.value(QStringLiteral("key")).toString() == key)
        {
            return group;
        }
    }
    return {};
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    QJsonArray files {
        fileEntry(QStringLiteral("before"), QStringLiteral("image"), QStringLiteral("artifacts/testdiff/before/view.png"), 10),
        fileEntry(QStringLiteral("after"), QStringLiteral("image"), QStringLiteral("artifacts/testdiff/after/view.png"), 20),
        fileEntry(QStringLiteral("diff"), QStringLiteral("image"), QStringLiteral("artifacts/testdiff/diff/view.png"), 30),
        fileEntry(QStringLiteral("before"), QStringLiteral("property"), QStringLiteral("artifacts/testdiff/before/props.json"), 11),
        fileEntry(QStringLiteral("after"), QStringLiteral("property"), QStringLiteral("artifacts/testdiff/after/props.json"), 21),
        fileEntry(QStringLiteral("diff"), QStringLiteral("performance"), QStringLiteral("artifacts/testdiff/diff/performance_timing.txt"), 31),
        fileEntry(QStringLiteral("diff"), QStringLiteral("log"), QStringLiteral("artifacts/testdiff/diff/ignored.log"), 41),
    };

    const QJsonObject index = occtdebug::TestdiffArtifactIndex::build(files);
    const QJsonObject counts = index.value(QStringLiteral("counts")).toObject();
    const QJsonObject imageCounts = counts.value(QStringLiteral("image")).toObject();
    const QJsonObject propertyCounts = counts.value(QStringLiteral("property")).toObject();
    const QJsonObject performanceCounts = counts.value(QStringLiteral("performance")).toObject();
    const QJsonArray groups = index.value(QStringLiteral("groups")).toArray();
    const QJsonObject imageGroup = findGroup(groups, QStringLiteral("image"), QStringLiteral("view"));
    const QJsonObject propertyGroup = findGroup(groups, QStringLiteral("property"), QStringLiteral("props"));
    const QJsonObject performanceGroup = findGroup(groups, QStringLiteral("performance"), QStringLiteral("performance_timing"));

    if (!expect(index.value(QStringLiteral("schema_version")).toInt() == 1, "schema version mismatch")
        || !expect(index.value(QStringLiteral("supported_kinds")).toArray().size() == 3, "supported kind count mismatch")
        || !expect(imageCounts.value(QStringLiteral("before")).toInt() == 1, "image before count mismatch")
        || !expect(imageCounts.value(QStringLiteral("after")).toInt() == 1, "image after count mismatch")
        || !expect(imageCounts.value(QStringLiteral("diff")).toInt() == 1, "image diff count mismatch")
        || !expect(propertyCounts.value(QStringLiteral("total")).toInt() == 2, "property total count mismatch")
        || !expect(performanceCounts.value(QStringLiteral("diff")).toInt() == 1, "performance diff count mismatch"))
    {
        return 1;
    }

    if (!expect(imageGroup.value(QStringLiteral("status")).toString() == QStringLiteral("paired_with_diff"),
                "image group status mismatch")
        || !expect(propertyGroup.value(QStringLiteral("status")).toString() == QStringLiteral("paired"),
                  "property group status mismatch")
        || !expect(performanceGroup.value(QStringLiteral("status")).toString() == QStringLiteral("diff_only"),
                  "performance group status mismatch")
        || !expect(groups.size() == 3, "unsupported log artifact should not be indexed")
        || !expect(index.value(QStringLiteral("strategy")).toObject().contains(QStringLiteral("image")),
                  "strategy is missing image entry"))
    {
        return 2;
    }

    QTextStream(stdout) << "TESTDIFF_ARTIFACT_INDEX_SMOKE_OK\n";
    return 0;
}
