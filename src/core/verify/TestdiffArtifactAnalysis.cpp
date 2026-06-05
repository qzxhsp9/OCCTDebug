#include "core/verify/TestdiffArtifactAnalysis.h"

#include "core/verify/TestdiffGenerationPolicy.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

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

QString safeArtifactPath(const QString& workspaceRoot, const QJsonObject& group, const QStringList& roles)
{
    for (const QString& role : roles)
    {
        const QString path = normalizedPath(group.value(role).toString());
        if (path.isEmpty()
            || QFileInfo(path).isAbsolute()
            || path == QStringLiteral("..")
            || path.startsWith(QStringLiteral("../")))
        {
            continue;
        }
        const QString absolutePath = QDir(workspaceRoot).filePath(path);
        if (QFileInfo::exists(absolutePath))
        {
            return path;
        }
    }
    return {};
}

QString readTextFile(const QString& workspaceRoot, const QString& relativePath, int maxBytes = 64 * 1024)
{
    if (relativePath.isEmpty())
    {
        return {};
    }
    QFile file(QDir(workspaceRoot).filePath(relativePath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return {};
    }
    return QString::fromUtf8(file.read(maxBytes));
}

QJsonObject jsonSummary(const QString& workspaceRoot, const QString& relativePath)
{
    const QString text = readTextFile(workspaceRoot, relativePath);
    if (text.isEmpty())
    {
        return {
            {QStringLiteral("available"), false},
            {QStringLiteral("path"), relativePath},
        };
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &parseError);
    QJsonObject out {
        {QStringLiteral("available"), true},
        {QStringLiteral("path"), relativePath},
        {QStringLiteral("valid_json"), parseError.error == QJsonParseError::NoError},
    };
    if (parseError.error != QJsonParseError::NoError)
    {
        out.insert(QStringLiteral("error"), parseError.errorString());
        return out;
    }

    if (doc.isObject())
    {
        QJsonArray keys;
        const QJsonObject object = doc.object();
        for (auto it = object.constBegin(); it != object.constEnd(); ++it)
        {
            keys.append(it.key());
            if (keys.size() >= 32)
            {
                break;
            }
        }
        out.insert(QStringLiteral("json_type"), QStringLiteral("object"));
        out.insert(QStringLiteral("top_level_key_count"), object.size());
        out.insert(QStringLiteral("top_level_keys"), keys);
    }
    else if (doc.isArray())
    {
        out.insert(QStringLiteral("json_type"), QStringLiteral("array"));
        out.insert(QStringLiteral("array_size"), doc.array().size());
    }
    else
    {
        out.insert(QStringLiteral("json_type"), QStringLiteral("scalar"));
    }
    return out;
}

QJsonArray performanceMetrics(const QString& text)
{
    QJsonArray metrics;
    const QRegularExpression namedMetric(
        QStringLiteral(R"(^\s*([A-Za-z_][A-Za-z0-9_.\-/]*)\s*[:= ]+\s*([+-]?\d+(?:\.\d+)?)\s*([A-Za-z%/]+)?\s*$)"));
    const QRegularExpression valueOnlyMetric(
        QStringLiteral(R"(^\s*([+-]?\d+(?:\.\d+)?)\s*([A-Za-z%/]+)?\s*$)"));
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("\\r?\\n")));
    for (int index = 0; index < lines.size() && metrics.size() < 16; ++index)
    {
        const QString line = lines.at(index).trimmed();
        if (line.isEmpty())
        {
            continue;
        }

        QRegularExpressionMatch match = namedMetric.match(line);
        QString name;
        QString value;
        QString unit;
        if (match.hasMatch())
        {
            name = match.captured(1);
            value = match.captured(2);
            unit = match.captured(3);
        }
        else
        {
            match = valueOnlyMetric.match(line);
            if (!match.hasMatch())
            {
                continue;
            }
            name = QStringLiteral("value");
            value = match.captured(1);
            unit = match.captured(2);
        }

        metrics.append(QJsonObject {
            {QStringLiteral("name"), name},
            {QStringLiteral("value"), value.toDouble()},
            {QStringLiteral("raw_value"), value},
            {QStringLiteral("unit"), unit},
            {QStringLiteral("line"), index + 1},
        });
    }
    return metrics;
}

QJsonObject imageAnalysis(const QJsonObject& group)
{
    return {
        {QStringLiteral("diff_supplied_by_runner"), group.contains(QStringLiteral("diff"))},
        {QStringLiteral("before_supplied"), group.contains(QStringLiteral("before"))},
        {QStringLiteral("after_supplied"), group.contains(QStringLiteral("after"))},
        {QStringLiteral("strategy"), QStringLiteral("Index image artifacts supplied by runner; pixel-level comparison remains an external testdiff responsibility.")},
    };
}

QJsonObject propertyAnalysis(const QString& workspaceRoot, const QJsonObject& group)
{
    const QString path = safeArtifactPath(workspaceRoot, group, {
        QStringLiteral("diff"),
        QStringLiteral("after"),
        QStringLiteral("before"),
    });
    return {
        {QStringLiteral("source_path"), path},
        {QStringLiteral("json"), jsonSummary(workspaceRoot, path)},
        {QStringLiteral("strategy"), QStringLiteral("Parse JSON property artifacts when available; XML/YAML/CSV/BREP remain indexed-only in this stage.")},
    };
}

QJsonObject performanceAnalysis(const QString& workspaceRoot, const QJsonObject& group)
{
    const QString path = safeArtifactPath(workspaceRoot, group, {
        QStringLiteral("diff"),
        QStringLiteral("after"),
        QStringLiteral("before"),
    });
    const QString text = readTextFile(workspaceRoot, path);
    return {
        {QStringLiteral("source_path"), path},
        {QStringLiteral("metrics"), performanceMetrics(text)},
        {QStringLiteral("strategy"), QStringLiteral("Extract simple numeric metrics from text artifacts; full testdiff performance semantics remain a later parser step.")},
    };
}
} // namespace

QJsonObject TestdiffArtifactAnalysis::build(const QString& workspaceRoot, const QJsonObject& artifactIndex)
{
    QJsonArray groups;
    int imageDiffSupplied = 0;
    int propertyJsonParsed = 0;
    int performanceMetricCount = 0;

    for (const QJsonValue& value : artifactIndex.value(QStringLiteral("groups")).toArray())
    {
        const QJsonObject group = value.toObject();
        const QString kind = group.value(QStringLiteral("kind")).toString();
        QJsonObject analysis;
        if (kind == QStringLiteral("image"))
        {
            analysis = imageAnalysis(group);
            if (analysis.value(QStringLiteral("diff_supplied_by_runner")).toBool())
            {
                ++imageDiffSupplied;
            }
        }
        else if (kind == QStringLiteral("property"))
        {
            analysis = propertyAnalysis(workspaceRoot, group);
            if (analysis.value(QStringLiteral("json")).toObject().value(QStringLiteral("valid_json")).toBool())
            {
                ++propertyJsonParsed;
            }
        }
        else if (kind == QStringLiteral("performance"))
        {
            analysis = performanceAnalysis(workspaceRoot, group);
            performanceMetricCount += analysis.value(QStringLiteral("metrics")).toArray().size();
        }
        else
        {
            continue;
        }

        groups.append(QJsonObject {
            {QStringLiteral("kind"), kind},
            {QStringLiteral("key"), group.value(QStringLiteral("key")).toString()},
            {QStringLiteral("status"), group.value(QStringLiteral("status")).toString()},
            {QStringLiteral("analysis"), analysis},
        });
    }

    QJsonObject result {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("summary"), QJsonObject {
             {QStringLiteral("groups"), groups.size()},
             {QStringLiteral("image_diff_supplied_by_runner"), imageDiffSupplied},
             {QStringLiteral("property_json_parsed"), propertyJsonParsed},
             {QStringLiteral("performance_metrics"), performanceMetricCount},
         }},
        {QStringLiteral("groups"), groups},
        {QStringLiteral("limits"), QJsonObject {
             {QStringLiteral("pixel_diff_generated"), false},
             {QStringLiteral("property_structural_diff_generated"), false},
             {QStringLiteral("performance_trend_generated"), false},
         }},
    };
    result.insert(QStringLiteral("generation_policy"), TestdiffGenerationPolicy::build(artifactIndex, result));
    return result;
}
} // namespace occtdebug
