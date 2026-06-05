#include "core/geometry/TopologyCompareArtifact.h"

#include "core/geometry/TopologySignature.h"

#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPair>
#include <QSaveFile>
#include <QVector>

namespace occtdebug
{
namespace
{
QString normalizedPath(QString path)
{
    path.replace(QLatin1Char('\\'), QLatin1Char('/'));
    while (path.startsWith(QStringLiteral("./")))
    {
        path.remove(0, 2);
    }
    return QDir::cleanPath(path);
}

QJsonObject unavailableObject(const QString& reason)
{
    return {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("available"), false},
        {QStringLiteral("status"), QStringLiteral("unavailable")},
        {QStringLiteral("summary_text"), reason},
    };
}

QJsonObject loadJsonObject(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return {};
    }
    return QJsonDocument::fromJson(file.readAll()).object();
}

QString caseRelativeOrFileName(const QString& caseRoot, const QString& path)
{
    const QFileInfo info(path);
    if (!info.exists())
    {
        return QString();
    }

    const QString absolutePath = info.absoluteFilePath();
    const QString relative = normalizedPath(QDir(caseRoot).relativeFilePath(absolutePath));
    if (!relative.startsWith(QStringLiteral("../")) && !QFileInfo(relative).isAbsolute())
    {
        return relative;
    }
    return info.fileName();
}

bool writeJsonObject(const QString& path, const QJsonObject& object, QString* error)
{
    QDir dir;
    if (!dir.mkpath(QFileInfo(path).absolutePath()))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot create topology compare directory: %1").arg(QFileInfo(path).absolutePath());
        }
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot write topology compare artifact %1: %2").arg(path, file.errorString());
        }
        return false;
    }
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    if (!file.commit())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot commit topology compare artifact %1: %2").arg(path, file.errorString());
        }
        return false;
    }
    return true;
}

QJsonObject compactCompareObject(QJsonObject raw, const QString& artifactPath, const QString& source)
{
    const QJsonObject matchSummary = raw.value(QStringLiteral("summary")).toObject();
    const QString status = raw.value(QStringLiteral("status")).toString(
        matchSummary.value(QStringLiteral("status")).toString(QStringLiteral("unknown")));
    const int matched = matchSummary.value(QStringLiteral("matched")).toInt();
    const int unmatchedBefore = matchSummary.value(QStringLiteral("unmatched_before")).toInt();
    const int unmatchedAfter = matchSummary.value(QStringLiteral("unmatched_after")).toInt();
    const int exactMatches = matchSummary.value(QStringLiteral("exact_hash_matches")).toInt();
    const int approximateMatches = matchSummary.value(QStringLiteral("approximate_matches")).toInt();

    QString summaryText = raw.value(QStringLiteral("summary_text")).toString();
    if (summaryText.isEmpty())
    {
        summaryText = QStringLiteral("topology %1: matched=%2 exact=%3 approximate=%4 unmatched_before=%5 unmatched_after=%6")
            .arg(status)
            .arg(matched)
            .arg(exactMatches)
            .arg(approximateMatches)
            .arg(unmatchedBefore)
            .arg(unmatchedAfter);
    }

    QJsonObject out {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("available"), true},
        {QStringLiteral("source"), source},
        {QStringLiteral("status"), status},
        {QStringLiteral("summary_text"), summaryText},
        {QStringLiteral("before_source"), raw.value(QStringLiteral("before_source")).toString()},
        {QStringLiteral("after_source"), raw.value(QStringLiteral("after_source")).toString()},
        {QStringLiteral("minimum_score"), raw.value(QStringLiteral("minimum_score")).toDouble()},
        {QStringLiteral("basis"), raw.value(QStringLiteral("basis")).toString()},
        {QStringLiteral("counts_delta"), raw.value(QStringLiteral("counts_delta")).toObject()},
        {QStringLiteral("match_summary"), matchSummary},
        {QStringLiteral("matches"), raw.value(QStringLiteral("matches")).toArray()},
        {QStringLiteral("unmatched_before"), raw.value(QStringLiteral("unmatched_before")).toArray()},
        {QStringLiteral("unmatched_after"), raw.value(QStringLiteral("unmatched_after")).toArray()},
    };
    if (!artifactPath.isEmpty())
    {
        out.insert(QStringLiteral("artifact"), normalizedPath(artifactPath));
    }
    const QString beforeSignature = normalizedPath(raw.value(QStringLiteral("before_signature")).toString());
    if (!beforeSignature.isEmpty())
    {
        out.insert(QStringLiteral("before_signature"), beforeSignature);
    }
    const QString afterSignature = normalizedPath(raw.value(QStringLiteral("after_signature")).toString());
    if (!afterSignature.isEmpty())
    {
        out.insert(QStringLiteral("after_signature"), afterSignature);
    }
    return out;
}

QJsonObject compareFromSignaturePair(const QDir& root, const QString& beforeRelative, const QString& afterRelative)
{
    const QJsonObject before = loadJsonObject(root.filePath(beforeRelative));
    const QJsonObject after = loadJsonObject(root.filePath(afterRelative));
    if (before.isEmpty() || after.isEmpty())
    {
        return {};
    }

    QJsonObject compare = TopologySignature::compare(before, after);
    compare.insert(QStringLiteral("before_signature"), beforeRelative);
    compare.insert(QStringLiteral("after_signature"), afterRelative);
    return compactCompareObject(compare, QString(), QStringLiteral("signature_pair"));
}
} // namespace

QString TopologyCompareArtifact::defaultArtifactRelativePath()
{
    return QStringLiteral("artifacts/topology_compare.json");
}

QStringList TopologyCompareArtifact::compareArtifactCandidates()
{
    return {
        QStringLiteral("artifacts/topology_compare.json"),
        QStringLiteral("artifacts/topology/topology_compare.json"),
        QStringLiteral("artifacts/geometry/topology_compare.json"),
    };
}

QJsonObject TopologyCompareArtifact::buildFromSignatureFiles(const QString& beforeSignaturePath,
                                                             const QString& afterSignaturePath,
                                                             const QString& caseRoot,
                                                             QString* error)
{
    const QFileInfo beforeInfo(beforeSignaturePath);
    const QFileInfo afterInfo(afterSignaturePath);
    if (!beforeInfo.exists() || !beforeInfo.isFile())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("before topology signature does not exist: %1").arg(beforeSignaturePath);
        }
        return {};
    }
    if (!afterInfo.exists() || !afterInfo.isFile())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("after topology signature does not exist: %1").arg(afterSignaturePath);
        }
        return {};
    }

    const QJsonObject before = loadJsonObject(beforeInfo.absoluteFilePath());
    const QJsonObject after = loadJsonObject(afterInfo.absoluteFilePath());
    if (before.isEmpty() || after.isEmpty())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("failed to read topology signature JSON");
        }
        return {};
    }

    QJsonObject compare = TopologySignature::compare(before, after);
    compare.insert(QStringLiteral("schema_version"), 1);
    compare.insert(QStringLiteral("generated_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    compare.insert(QStringLiteral("source"), QStringLiteral("signature_file_pair"));
    compare.insert(QStringLiteral("before_signature"), caseRelativeOrFileName(caseRoot, beforeInfo.absoluteFilePath()));
    compare.insert(QStringLiteral("after_signature"), caseRelativeOrFileName(caseRoot, afterInfo.absoluteFilePath()));
    compare.insert(QStringLiteral("before_signature_source"), before.value(QStringLiteral("source")).toString());
    compare.insert(QStringLiteral("after_signature_source"), after.value(QStringLiteral("source")).toString());
    return compare;
}

QJsonObject TopologyCompareArtifact::writeForCase(const QString& caseRoot,
                                                  const QString& beforeSignaturePath,
                                                  const QString& afterSignaturePath,
                                                  QString* error)
{
    const QJsonObject compare = buildFromSignatureFiles(beforeSignaturePath, afterSignaturePath, caseRoot, error);
    if (compare.isEmpty())
    {
        return {};
    }

    const QString artifactRelativePath = defaultArtifactRelativePath();
    const QString artifactPath = QDir(caseRoot).filePath(artifactRelativePath);
    if (!writeJsonObject(artifactPath, compare, error))
    {
        return {};
    }
    return compactCompareObject(compare, artifactRelativePath, QStringLiteral("generated_compare_artifact"));
}

QJsonObject TopologyCompareArtifact::loadForCase(const QString& caseRoot)
{
    const QDir root(caseRoot);
    for (const QString& relativePath : compareArtifactCandidates())
    {
        const QString absolutePath = root.filePath(relativePath);
        if (!QFileInfo::exists(absolutePath))
        {
            continue;
        }
        const QJsonObject raw = loadJsonObject(absolutePath);
        if (!raw.isEmpty())
        {
            return compactCompareObject(raw, relativePath, QStringLiteral("compare_artifact"));
        }
    }

    const QVector<QPair<QString, QString>> signaturePairs {
        {QStringLiteral("artifacts/topology_signature_before.json"), QStringLiteral("artifacts/topology_signature_after.json")},
        {QStringLiteral("artifacts/topology/before_signature.json"), QStringLiteral("artifacts/topology/after_signature.json")},
        {QStringLiteral("artifacts/geometry/topology_before.json"), QStringLiteral("artifacts/geometry/topology_after.json")},
    };
    for (const auto& pair : signaturePairs)
    {
        if (QFileInfo::exists(root.filePath(pair.first)) && QFileInfo::exists(root.filePath(pair.second)))
        {
            const QJsonObject compare = compareFromSignaturePair(root, pair.first, pair.second);
            if (!compare.isEmpty())
            {
                return compare;
            }
        }
    }
    return unavailableObject(QStringLiteral("topology before/after comparison artifact is not available"));
}

QString TopologyCompareArtifact::summaryText(const QJsonObject& artifact)
{
    const QString summary = artifact.value(QStringLiteral("summary_text")).toString();
    if (!summary.isEmpty())
    {
        return summary;
    }
    if (!artifact.value(QStringLiteral("available")).toBool())
    {
        return QStringLiteral("topology before/after comparison artifact is not available");
    }
    const QJsonObject matchSummary = artifact.value(QStringLiteral("match_summary")).toObject();
    return QStringLiteral("topology %1: matched=%2 unmatched_before=%3 unmatched_after=%4")
        .arg(artifact.value(QStringLiteral("status")).toString(QStringLiteral("unknown")))
        .arg(matchSummary.value(QStringLiteral("matched")).toInt())
        .arg(matchSummary.value(QStringLiteral("unmatched_before")).toInt())
        .arg(matchSummary.value(QStringLiteral("unmatched_after")).toInt());
}
} // namespace occtdebug
