#include "core/evidence/EvidenceBundleWriter.h"

#include "core/geometry/TopologyCompareArtifact.h"
#include "core/verify/VerificationResultParser.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>

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
    return path;
}

QJsonValue normalizedJsonValue(const QJsonValue& value)
{
    if (value.isString())
    {
        return normalizedPath(value.toString());
    }
    if (value.isArray())
    {
        QJsonArray out;
        for (const QJsonValue& item : value.toArray())
        {
            out.append(normalizedJsonValue(item));
        }
        return out;
    }
    if (value.isObject())
    {
        QJsonObject out;
        const QJsonObject object = value.toObject();
        for (auto it = object.constBegin(); it != object.constEnd(); ++it)
        {
            out.insert(it.key(), normalizedJsonValue(it.value()));
        }
        return out;
    }
    return value;
}

QString categoryForEvidence(const EvidenceRecord& evidence)
{
    const QString type = evidence.type.toLower();
    const QString link = evidence.link.toLower();
    if (!evidence.logFile.isEmpty() || type.contains(QStringLiteral("draw")) || link.startsWith(QStringLiteral("logs/")) || link.contains(QStringLiteral("draw")))
    {
        return QStringLiteral("log");
    }
    if (!evidence.sourceFile.isEmpty() || evidence.sourceLine > 0 || !evidence.stackFrame.isEmpty()
        || type.contains(QStringLiteral("source")) || link.contains(QRegularExpression(QStringLiteral(":[0-9]+$"))))
    {
        return QStringLiteral("source");
    }
    if (!evidence.geometryObject.isEmpty() || type.contains(QStringLiteral("shape")) || type.contains(QStringLiteral("geometry")))
    {
        return QStringLiteral("geometry");
    }
    if (type.contains(QStringLiteral("patch")))
    {
        return QStringLiteral("patch");
    }
    if (type.contains(QStringLiteral("report")) || link.startsWith(QStringLiteral("report/")))
    {
        return QStringLiteral("report");
    }
    return QStringLiteral("artifact");
}

QJsonObject linkInfo(const QString& link, const QDir& caseRoot)
{
    QJsonObject out {
        {QStringLiteral("raw"), link},
        {QStringLiteral("normalized"), normalizedPath(link)},
    };

    if (link.trimmed().isEmpty())
    {
        out.insert(QStringLiteral("status"), QStringLiteral("empty"));
        return out;
    }

    if (link.contains(QStringLiteral("://")) || QFileInfo(link).isAbsolute())
    {
        out.insert(QStringLiteral("status"), QStringLiteral("blocked_absolute_or_external"));
        return out;
    }

    const QRegularExpression sourceRef(QStringLiteral("^(.+):(\\d+)$"));
    const QRegularExpressionMatch sourceMatch = sourceRef.match(link);
    if (sourceMatch.hasMatch() && !link.startsWith(QStringLiteral("logs/")) && !link.startsWith(QStringLiteral("artifacts/")))
    {
        out.insert(QStringLiteral("status"), QStringLiteral("source_reference"));
        out.insert(QStringLiteral("source_file"), normalizedPath(sourceMatch.captured(1)));
        out.insert(QStringLiteral("line"), sourceMatch.captured(2).toInt());
        return out;
    }

    const QString absoluteTarget = QDir::cleanPath(caseRoot.absoluteFilePath(link));
    out.insert(QStringLiteral("case_relative_path"), normalizedPath(link));
    out.insert(QStringLiteral("exists"), QFileInfo::exists(absoluteTarget));
    out.insert(QStringLiteral("status"), QFileInfo::exists(absoluteTarget) ? QStringLiteral("ok") : QStringLiteral("missing"));
    return out;
}

QJsonObject evidenceObject(const EvidenceRecord& evidence, const QDir& caseRoot, int index)
{
    const QJsonObject link = linkInfo(evidence.link, caseRoot);
    QJsonObject location;
    if (!evidence.sourceFile.isEmpty())
    {
        location.insert(QStringLiteral("source_file"), normalizedPath(evidence.sourceFile));
    }
    if (evidence.sourceLine > 0)
    {
        location.insert(QStringLiteral("source_line"), evidence.sourceLine);
    }
    if (!evidence.logFile.isEmpty())
    {
        location.insert(QStringLiteral("log_file"), normalizedPath(evidence.logFile));
    }
    if (evidence.logLine > 0)
    {
        location.insert(QStringLiteral("log_line"), evidence.logLine);
    }
    if (!evidence.stackFrame.isEmpty())
    {
        location.insert(QStringLiteral("stack_frame"), evidence.stackFrame);
    }
    if (!evidence.geometryObject.isEmpty())
    {
        location.insert(QStringLiteral("geometry_object"), evidence.geometryObject);
    }

    QJsonObject out {
        {QStringLiteral("id"), QStringLiteral("ev_%1").arg(index + 1, 4, 10, QLatin1Char('0'))},
        {QStringLiteral("type"), evidence.type},
        {QStringLiteral("title"), evidence.title},
        {QStringLiteral("summary"), evidence.summary},
        {QStringLiteral("category"), categoryForEvidence(evidence)},
        {QStringLiteral("link"), link},
    };
    if (!location.isEmpty())
    {
        out.insert(QStringLiteral("location"), location);
    }
    return out;
}

QJsonObject geometryCheckObject(const GeometryCheck& check)
{
    return QJsonObject {
        {QStringLiteral("name"), check.name},
        {QStringLiteral("status"), check.status},
        {QStringLiteral("note"), check.note},
    };
}

QJsonObject labelValueObject(const LabelValue& value)
{
    return QJsonObject {
        {QStringLiteral("label"), value.label},
        {QStringLiteral("value"), value.value},
    };
}

QJsonArray labelValueArray(const QVector<LabelValue>& values)
{
    QJsonArray array;
    for (const LabelValue& value : values)
    {
        array.append(labelValueObject(value));
    }
    return array;
}

QString itemValue(const QVector<LabelValue>& items, const QString& labelNeedle)
{
    const QString loweredNeedle = labelNeedle.toLower();
    for (const LabelValue& item : items)
    {
        if (item.label.toLower().contains(loweredNeedle))
        {
            return item.value;
        }
    }
    return QString();
}

QString readTextFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return QString();
    }
    return QString::fromUtf8(file.readAll());
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

QJsonObject testgridResultObject(const QDir& caseRoot)
{
    return loadJsonObject(caseRoot.filePath(QStringLiteral("artifacts/testgrid_result.json")));
}

QJsonArray verificationFailuresObject(const QJsonObject& testgridResult)
{
    QJsonArray out;
    for (const QJsonValue& value : testgridResult.value(QStringLiteral("failure_details")).toArray())
    {
        const QJsonObject failure = value.toObject();
        QJsonObject item {
            {QStringLiteral("type"), failure.value(QStringLiteral("type")).toString()},
            {QStringLiteral("name"), failure.value(QStringLiteral("name")).toString()},
            {QStringLiteral("status"), failure.value(QStringLiteral("status")).toString()},
            {QStringLiteral("summary"), failure.value(QStringLiteral("summary")).toString()},
        };
        const QString artifact = normalizedPath(failure.value(QStringLiteral("artifact")).toString());
        if (!artifact.isEmpty())
        {
            item.insert(QStringLiteral("artifact"), artifact);
        }
        out.append(item);
    }
    return out;
}

QJsonObject verificationTimingObject(const QJsonObject& testgridResult)
{
    const QJsonObject timing = testgridResult.value(QStringLiteral("timing")).toObject();
    QJsonArray entries;
    for (const QJsonValue& value : timing.value(QStringLiteral("entries")).toArray())
    {
        const QJsonObject entry = value.toObject();
        entries.append(QJsonObject {
            {QStringLiteral("name"), entry.value(QStringLiteral("name")).toString()},
            {QStringLiteral("elapsed_ms"), entry.value(QStringLiteral("elapsed_ms")).toDouble()},
            {QStringLiteral("status"), entry.value(QStringLiteral("status")).toString()},
        });
    }
    return QJsonObject {
        {QStringLiteral("total_elapsed_ms"), timing.value(QStringLiteral("total_elapsed_ms")).toDouble()},
        {QStringLiteral("summary"), timing.value(QStringLiteral("summary")).toString()},
        {QStringLiteral("entries"), entries},
    };
}

QJsonObject artifactIndexSummaryObject(const QJsonObject& artifactIndex)
{
    QJsonObject statusCounts {
        {QStringLiteral("paired_with_diff"), 0},
        {QStringLiteral("paired"), 0},
        {QStringLiteral("diff_only"), 0},
        {QStringLiteral("incomplete"), 0},
    };
    QJsonObject kindGroupCounts {
        {QStringLiteral("image"), 0},
        {QStringLiteral("property"), 0},
        {QStringLiteral("performance"), 0},
    };

    const QJsonArray groups = artifactIndex.value(QStringLiteral("groups")).toArray();
    for (const QJsonValue& value : groups)
    {
        const QJsonObject group = value.toObject();
        const QString status = group.value(QStringLiteral("status")).toString(QStringLiteral("incomplete"));
        statusCounts.insert(status, statusCounts.value(status).toInt() + 1);

        const QString kind = group.value(QStringLiteral("kind")).toString();
        if (!kind.isEmpty())
        {
            kindGroupCounts.insert(kind, kindGroupCounts.value(kind).toInt() + 1);
        }
    }

    return {
        {QStringLiteral("total_groups"), groups.size()},
        {QStringLiteral("status_counts"), statusCounts},
        {QStringLiteral("kind_group_counts"), kindGroupCounts},
        {QStringLiteral("available"), !groups.isEmpty()},
    };
}

QJsonObject artifactIndexObject(const QJsonObject& raw)
{
    if (raw.isEmpty())
    {
        return {};
    }

    QJsonArray supportedKinds;
    for (const QJsonValue& value : raw.value(QStringLiteral("supported_kinds")).toArray())
    {
        const QString kind = value.toString();
        if (!kind.isEmpty())
        {
            supportedKinds.append(kind);
        }
    }

    QJsonArray groups;
    for (const QJsonValue& value : raw.value(QStringLiteral("groups")).toArray())
    {
        const QJsonObject group = value.toObject();
        QJsonObject clean {
            {QStringLiteral("kind"), group.value(QStringLiteral("kind")).toString()},
            {QStringLiteral("key"), group.value(QStringLiteral("key")).toString()},
            {QStringLiteral("status"), group.value(QStringLiteral("status")).toString()},
        };
        for (const QString& role : {QStringLiteral("before"), QStringLiteral("after"), QStringLiteral("diff")})
        {
            const QString path = normalizedPath(group.value(role).toString());
            if (!path.isEmpty())
            {
                clean.insert(role, path);
            }
        }
        const QJsonObject bytes = group.value(QStringLiteral("bytes")).toObject();
        if (!bytes.isEmpty())
        {
            clean.insert(QStringLiteral("bytes"), bytes);
        }
        groups.append(clean);
    }

    QJsonObject out {
        {QStringLiteral("schema_version"), raw.value(QStringLiteral("schema_version")).toInt()},
        {QStringLiteral("supported_kinds"), supportedKinds},
        {QStringLiteral("counts"), raw.value(QStringLiteral("counts")).toObject()},
        {QStringLiteral("groups"), groups},
        {QStringLiteral("strategy"), raw.value(QStringLiteral("strategy")).toObject()},
    };
    out.insert(QStringLiteral("summary"), artifactIndexSummaryObject(out));
    return out;
}

QJsonObject testdiffArtifactsObject(const QJsonObject& testgridResult)
{
    const QJsonObject raw = testgridResult.value(QStringLiteral("testdiff_artifacts")).toObject();
    QJsonObject out {
        {QStringLiteral("entries_count"), raw.value(QStringLiteral("entries_count")).toInt()},
        {QStringLiteral("changed_count"), raw.value(QStringLiteral("changed_count")).toInt()},
        {QStringLiteral("failed_count"), raw.value(QStringLiteral("failed_count")).toInt()},
    };
    const QStringList pathKeys {
        QStringLiteral("summary"),
        QStringLiteral("command_stdout"),
        QStringLiteral("command_stderr"),
        QStringLiteral("before_result"),
        QStringLiteral("after_result"),
        QStringLiteral("two_stage_result"),
    };
    for (const QString& key : pathKeys)
    {
        const QString path = normalizedPath(raw.value(key).toString());
        if (!path.isEmpty())
        {
            out.insert(key, path);
        }
    }
    const QJsonObject counts = raw.value(QStringLiteral("artifact_counts")).toObject();
    if (!counts.isEmpty())
    {
        out.insert(QStringLiteral("artifact_counts"), counts);
    }
    const QJsonArray directories = raw.value(QStringLiteral("directories")).toArray();
    if (!directories.isEmpty())
    {
        QJsonArray cleanDirectories;
        for (const QJsonValue& value : directories)
        {
            const QJsonObject item = value.toObject();
            const QString path = normalizedPath(item.value(QStringLiteral("path")).toString());
            if (!path.isEmpty())
            {
                cleanDirectories.append(QJsonObject {
                    {QStringLiteral("role"), item.value(QStringLiteral("role")).toString()},
                    {QStringLiteral("path"), path},
                });
            }
        }
        out.insert(QStringLiteral("directories"), cleanDirectories);
    }
    const QJsonArray files = raw.value(QStringLiteral("artifact_files")).toArray();
    if (!files.isEmpty())
    {
        QJsonArray cleanFiles;
        for (const QJsonValue& value : files)
        {
            const QJsonObject item = value.toObject();
            const QString path = normalizedPath(item.value(QStringLiteral("path")).toString());
            if (!path.isEmpty())
            {
                cleanFiles.append(QJsonObject {
                    {QStringLiteral("role"), item.value(QStringLiteral("role")).toString()},
                    {QStringLiteral("kind"), item.value(QStringLiteral("kind")).toString()},
                    {QStringLiteral("path"), path},
                    {QStringLiteral("bytes"), item.value(QStringLiteral("bytes")).toDouble()},
                });
            }
        }
        out.insert(QStringLiteral("artifact_files"), cleanFiles);
    }
    const QJsonObject artifactIndex = artifactIndexObject(raw.value(QStringLiteral("artifact_index")).toObject());
    if (!artifactIndex.isEmpty())
    {
        out.insert(QStringLiteral("artifact_index"), artifactIndex);
        out.insert(QStringLiteral("artifact_index_summary"), artifactIndex.value(QStringLiteral("summary")).toObject());
    }
    const QJsonObject artifactAnalysis =
        normalizedJsonValue(raw.value(QStringLiteral("artifact_analysis"))).toObject();
    if (!artifactAnalysis.isEmpty())
    {
        out.insert(QStringLiteral("artifact_analysis"), artifactAnalysis);
    }
    out.insert(QStringLiteral("truncated"), raw.value(QStringLiteral("truncated")).toBool(false));
    return out;
}

QJsonArray comparisonRows(const TestgridComparison& comparison)
{
    QJsonArray rows;
    for (const TestgridComparisonRow& row : comparison.rows)
    {
        rows.append(QJsonObject {
            {QStringLiteral("module"), row.module},
            {QStringLiteral("before_fail_count"), row.beforeFailCount},
            {QStringLiteral("after_fail_count"), row.afterFailCount},
            {QStringLiteral("fail_delta"), row.failDelta},
            {QStringLiteral("status"), row.status},
        });
    }
    return rows;
}

QJsonObject verificationComparisonObject(const CaseManifest& manifest, const QDir& caseRoot)
{
    const QString beforeRelative = QStringLiteral("verification/testgrid_before.txt");
    const QString afterRelative = QStringLiteral("verification/testgrid_after.txt");
    const QString beforePath = caseRoot.filePath(beforeRelative);
    const QString afterPath = caseRoot.filePath(afterRelative);
    const QVector<TestgridRow> beforeRows = VerificationResultParser::parseTestgridText(readTextFile(beforePath));
    QVector<TestgridRow> afterRows = VerificationResultParser::parseTestgridText(readTextFile(afterPath));
    if (afterRows.isEmpty())
    {
        afterRows = manifest.testgridRows;
    }
    const TestgridComparison comparison = VerificationResultParser::compareTestgridRows(beforeRows, afterRows);
    return QJsonObject {
        {QStringLiteral("available"), comparison.isAvailable()},
        {QStringLiteral("status"), !comparison.isAvailable() ? QStringLiteral("unavailable") : (comparison.hasRegression() ? QStringLiteral("regressed") : QStringLiteral("not_regressed"))},
        {QStringLiteral("summary"), comparison.summaryText()},
        {QStringLiteral("fail_delta"), comparison.failDelta},
        {QStringLiteral("before_summary"), QFileInfo::exists(beforePath) ? beforeRelative : QString()},
        {QStringLiteral("after_summary"), QFileInfo::exists(afterPath) ? afterRelative : QString()},
        {QStringLiteral("rows"), comparisonRows(comparison)},
    };
}
} // namespace

QJsonObject EvidenceBundleWriter::buildBundle(const CaseManifest& manifest, const QString& caseRoot)
{
    const QDir root(caseRoot);
    const QJsonObject testgridResult = testgridResultObject(root);
    QJsonArray records;
    QJsonArray sourceLinks;
    QJsonArray logLinks;
    QJsonArray geometryLinks;
    QJsonArray artifactLinks;

    for (int index = 0; index < manifest.evidenceItems.size(); ++index)
    {
        const EvidenceRecord& evidence = manifest.evidenceItems[index];
        const QJsonObject record = evidenceObject(evidence, root, index);
        records.append(record);

        const QString category = record.value(QStringLiteral("category")).toString();
        if (category == QStringLiteral("source"))
        {
            sourceLinks.append(record);
        }
        else if (category == QStringLiteral("log"))
        {
            logLinks.append(record);
        }
        else if (category == QStringLiteral("geometry"))
        {
            geometryLinks.append(record);
        }
        else
        {
            artifactLinks.append(record);
        }
    }

    QJsonArray geometryChecks;
    for (const GeometryCheck& check : manifest.geometryChecks)
    {
        geometryChecks.append(geometryCheckObject(check));
    }

    QJsonArray verificationItems;
    for (const LabelValue& item : manifest.verificationItems)
    {
        verificationItems.append(labelValueObject(item));
    }

    return QJsonObject {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("generated_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {QStringLiteral("case_id"), manifest.caseId},
        {QStringLiteral("title"), manifest.title},
        {QStringLiteral("status"), manifest.status},
        {QStringLiteral("summary"), manifest.evidenceSummary},
        {QStringLiteral("records"), records},
        {QStringLiteral("source_links"), sourceLinks},
        {QStringLiteral("log_links"), logLinks},
        {QStringLiteral("geometry_links"), geometryLinks},
        {QStringLiteral("artifact_links"), artifactLinks},
        {QStringLiteral("geometry_checks"), geometryChecks},
        {QStringLiteral("geometry_diff"), TopologyCompareArtifact::loadForCase(caseRoot)},
        {QStringLiteral("verification_items"), verificationItems},
        {QStringLiteral("verification_comparison"), verificationComparisonObject(manifest, root)},
        {QStringLiteral("verification_failures"), verificationFailuresObject(testgridResult)},
        {QStringLiteral("verification_timing"), verificationTimingObject(testgridResult)},
        {QStringLiteral("testdiff_artifacts"), testdiffArtifactsObject(testgridResult)},
        {QStringLiteral("diagnosis"), QJsonObject {
             {QStringLiteral("summary"), manifest.diagnosis},
             {QStringLiteral("confidence"), manifest.diagnosisConfidence},
         }},
        {QStringLiteral("patch"), QJsonObject {
             {QStringLiteral("review_status"), manifest.patchReviewStatus},
             {QStringLiteral("review_items"), labelValueArray(manifest.patchReviewItems)},
             {QStringLiteral("generation_status"), itemValue(manifest.verificationItems, QStringLiteral("patch generation"))},
             {QStringLiteral("apply_status"), manifest.patchApplyStatus},
             {QStringLiteral("dry_run_status"), itemValue(manifest.verificationItems, QStringLiteral("patch dry-run"))},
             {QStringLiteral("apply_log"), manifest.patchApplyLog},
             {QStringLiteral("signoff_status"), manifest.patchSignoffStatus},
             {QStringLiteral("signoff_note"), manifest.patchSignoffNote},
         }},
    };
}

bool EvidenceBundleWriter::writeBundle(const CaseManifest& manifest, const QString& caseRoot, const QString& outputPath, QString* error)
{
    const QFileInfo outputInfo(outputPath);
    QDir dir;
    if (!dir.mkpath(outputInfo.absolutePath()))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot create evidence bundle directory: %1").arg(outputInfo.absolutePath());
        }
        return false;
    }

    QSaveFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot write evidence bundle %1: %2").arg(outputPath, file.errorString());
        }
        return false;
    }

    file.write(QJsonDocument(buildBundle(manifest, caseRoot)).toJson(QJsonDocument::Indented));
    if (!file.commit())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot commit evidence bundle %1: %2").arg(outputPath, file.errorString());
        }
        return false;
    }
    return true;
}
} // namespace occtdebug
