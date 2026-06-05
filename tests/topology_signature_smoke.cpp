#include "core/geometry/TopologySignature.h"

#include <BRepPrimAPI_MakeBox.hxx>
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
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    const TopoDS_Shape shape = BRepPrimAPI_MakeBox(10.0, 20.0, 30.0).Shape();
    const QJsonObject signature = occtdebug::TopologySignature::build(shape, QStringLiteral("demo_box"), QStringLiteral("E1"));

    if (!expect(signature.value(QStringLiteral("schema_version")).toInt() == 1, "schema_version mismatch")
        || !expect(signature.value(QStringLiteral("basis")).toString().contains(QStringLiteral("BRepTools::Write")), "basis missing")
        || !expect(signature.value(QStringLiteral("source")).toString() == QStringLiteral("demo_box"), "source mismatch"))
    {
        return 1;
    }

    const QJsonObject counts = signature.value(QStringLiteral("counts")).toObject();
    if (!expect(counts.value(QStringLiteral("edge")).toInt() > 0, "edge count missing")
        || !expect(counts.value(QStringLiteral("face")).toInt() > 0, "face count missing")
        || !expect(counts.value(QStringLiteral("solid")).toInt() > 0, "solid count missing"))
    {
        return 2;
    }

    const QJsonArray records = signature.value(QStringLiteral("records")).toArray();
    if (!expect(!records.isEmpty(), "records are empty"))
    {
        return 3;
    }

    QString error;
    const QJsonObject edge = occtdebug::TopologySignature::objectSignature(shape, QStringLiteral("E1"), &error);
    if (!expect(!edge.isEmpty(), "E1 signature missing")
        || !expect(edge.value(QStringLiteral("object_id")).toString() == QStringLiteral("E1"), "E1 object_id mismatch")
        || !expect(edge.value(QStringLiteral("brep_sha256")).toString().size() == 64, "E1 sha256 length mismatch")
        || !expect(edge.value(QStringLiteral("stable_id")).toString().startsWith(QStringLiteral("E1#")), "E1 stable_id missing")
        || !expect(edge.value(QStringLiteral("geometry")).toObject().value(QStringLiteral("available")).toBool(), "E1 geometry descriptor missing")
        || !expect(edge.value(QStringLiteral("subshape_counts")).toObject().contains(QStringLiteral("vertex")), "E1 subshape counts missing"))
    {
        return 4;
    }

    const QString stableId = occtdebug::TopologySignature::stableIdForObject(shape, QStringLiteral("E1"), &error);
    if (!expect(stableId == edge.value(QStringLiteral("stable_id")).toString(), "stable id helper mismatch"))
    {
        return 5;
    }

    const QJsonObject identicalMatch = occtdebug::TopologySignature::compare(
        signature,
        occtdebug::TopologySignature::build(shape, QStringLiteral("demo_box_after")));
    const QJsonObject identicalSummary = identicalMatch.value(QStringLiteral("summary")).toObject();
    if (!expect(identicalMatch.value(QStringLiteral("schema_version")).toInt() == 1, "compare schema mismatch")
        || !expect(identicalSummary.value(QStringLiteral("status")).toString() == QStringLiteral("stable"), "identical compare status mismatch")
        || !expect(identicalSummary.value(QStringLiteral("matched")).toInt() == records.size(), "identical compare should match every record")
        || !expect(identicalSummary.value(QStringLiteral("unmatched_before")).toInt() == 0, "identical compare unmatched_before mismatch")
        || !expect(identicalSummary.value(QStringLiteral("exact_hash_matches")).toInt() > 0, "identical compare exact hash missing"))
    {
        return 6;
    }

    const TopoDS_Shape changedShape = BRepPrimAPI_MakeBox(10.0, 20.0, 31.0).Shape();
    const QJsonObject changedMatch = occtdebug::TopologySignature::compare(
        signature,
        occtdebug::TopologySignature::build(changedShape, QStringLiteral("demo_box_changed")));
    const QJsonObject changedSummary = changedMatch.value(QStringLiteral("summary")).toObject();
    if (!expect(changedSummary.value(QStringLiteral("matched")).toInt() > 0, "changed compare should keep some matches")
        || !expect(changedSummary.value(QStringLiteral("status")).toString() != QStringLiteral("stable"), "changed compare should not be stable")
        || !expect(changedMatch.value(QStringLiteral("counts_delta")).isObject(), "changed compare counts_delta missing"))
    {
        return 7;
    }
    bool foundGeometryHint = false;
    for (const QJsonValue& value : changedMatch.value(QStringLiteral("matches")).toArray())
    {
        const QString strategy = value.toObject().value(QStringLiteral("strategy")).toString();
        foundGeometryHint = foundGeometryHint
            || strategy.contains(QStringLiteral("measure"))
            || strategy.contains(QStringLiteral("bbox"))
            || strategy.contains(QStringLiteral("subshape_counts"));
    }
    if (!expect(foundGeometryHint, "changed compare should use local geometry or subshape count hints"))
    {
        return 9;
    }

    error.clear();
    const QJsonObject missing = occtdebug::TopologySignature::objectSignature(shape, QStringLiteral("E999999"), &error);
    if (!expect(missing.isEmpty(), "missing object should not produce signature")
        || !expect(!error.isEmpty(), "missing object should explain failure"))
    {
        return 10;
    }

    QTextStream(stdout) << "TOPOLOGY_SIGNATURE_SMOKE_OK\n";
    return 0;
}
