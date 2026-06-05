#include "core/geometry/TopologyCompareArtifact.h"
#include "core/geometry/TopologySignature.h"

#include <BRepPrimAPI_MakeBox.hxx>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
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

bool writeJson(const QString& path, const QJsonObject& object)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    return true;
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
    if (!expect(root.mkpath(QStringLiteral("artifacts")), "failed to create artifacts directory"))
    {
        return 2;
    }

    const QJsonObject before = occtdebug::TopologySignature::build(
        BRepPrimAPI_MakeBox(10.0, 20.0, 30.0).Shape(),
        QStringLiteral("before_box"));
    const QJsonObject after = occtdebug::TopologySignature::build(
        BRepPrimAPI_MakeBox(10.0, 20.0, 31.0).Shape(),
        QStringLiteral("after_box"));
    const QString beforePath = root.filePath(QStringLiteral("artifacts/topology_signature_before.json"));
    const QString afterPath = root.filePath(QStringLiteral("artifacts/topology_signature_after.json"));
    if (!expect(writeJson(beforePath, before), "failed to write before signature")
        || !expect(writeJson(afterPath, after), "failed to write after signature"))
    {
        return 3;
    }

    QString error;
    const QJsonObject generated = occtdebug::TopologyCompareArtifact::writeForCase(
        workspace.path(),
        beforePath,
        afterPath,
        &error);
    if (!expect(!generated.isEmpty(), "generated compare artifact is empty"))
    {
        QTextStream(stderr) << error << "\n";
        return 4;
    }
    if (!expect(generated.value(QStringLiteral("artifact")).toString() == QStringLiteral("artifacts/topology_compare.json"), "artifact path mismatch")
        || !expect(generated.value(QStringLiteral("available")).toBool(), "generated compare is unavailable")
        || !expect(generated.value(QStringLiteral("match_summary")).toObject().value(QStringLiteral("matched")).toInt() > 0, "generated compare has no matches"))
    {
        return 5;
    }

    const QJsonObject loaded = occtdebug::TopologyCompareArtifact::loadForCase(workspace.path());
    const QString loadedText = QString::fromUtf8(QJsonDocument(loaded).toJson(QJsonDocument::Compact));
    if (!expect(loaded.value(QStringLiteral("available")).toBool(), "loaded compare is unavailable")
        || !expect(occtdebug::TopologyCompareArtifact::summaryText(loaded).contains(QStringLiteral("topology")), "summary text mismatch")
        || !expect(!loadedText.contains(workspace.path()), "topology compare leaked absolute case path"))
    {
        return 6;
    }

    QTextStream(stdout) << "TOPOLOGY_COMPARE_ARTIFACT_SMOKE_OK\n";
    return 0;
}
