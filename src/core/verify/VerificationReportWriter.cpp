#include "core/verify/VerificationReportWriter.h"

#include "core/geometry/TopologyCompareArtifact.h"
#include "core/verify/VerificationResultParser.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStringConverter>
#include <QTextStream>

namespace occtdebug
{
namespace
{
struct TestgridTotals
{
    int run = 0;
    int pass = 0;
    int fail = 0;
};

QString normalizedPath(QString path)
{
    if (path.trimmed().isEmpty())
    {
        return QString();
    }
    path.replace(QLatin1Char('\\'), QLatin1Char('/'));
    while (path.startsWith(QStringLiteral("./")))
    {
        path.remove(0, 2);
    }
    return QDir::cleanPath(path);
}

QString sanitized(QString value)
{
    value.replace(QLatin1Char('\\'), QLatin1Char('/'));
    const QRegularExpression drivePath(QStringLiteral(R"(\b[A-Za-z]:/[^\r\n`|<>"']+)"));
    const QRegularExpression uncPath(QStringLiteral(R"((^|[\s(])//[^ \t\r\n`|<>"']+)"));
    value.replace(drivePath, QStringLiteral("<local-path>"));
    value.replace(uncPath, QStringLiteral("\\1<local-path>"));
    return value;
}

QString escapedCell(QString value)
{
    value = sanitized(value);
    value.replace(QLatin1Char('|'), QStringLiteral("\\|"));
    value.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
    return value;
}

QJsonValue sanitizedJsonValue(const QJsonValue& value);

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

QJsonArray labelValueArray(const QVector<LabelValue>& values)
{
    QJsonArray array;
    for (const LabelValue& value : values)
    {
        array.append(QJsonObject {
            {QStringLiteral("label"), value.label},
            {QStringLiteral("value"), sanitized(value.value)},
        });
    }
    return array;
}

QString reviewStatusState(const QString& reviewStatus)
{
    const QString lowered = reviewStatus.trimmed().toLower();
    if (lowered == QStringLiteral("approved"))
    {
        return QStringLiteral("approved");
    }
    if (lowered == QStringLiteral("rejected"))
    {
        return QStringLiteral("rejected");
    }
    if (lowered == QStringLiteral("needs review"))
    {
        return QStringLiteral("needs_review");
    }
    if (lowered == QStringLiteral("draft") || lowered.isEmpty())
    {
        return QStringLiteral("draft");
    }
    return QStringLiteral("unknown");
}

QJsonArray patchReviewItemsArray(const QVector<LabelValue>& values)
{
    QJsonArray array;
    for (const LabelValue& value : values)
    {
        array.append(QJsonObject {
            {QStringLiteral("step"), sanitized(value.label)},
            {QStringLiteral("state"), sanitized(value.value)},
        });
    }
    return array;
}

QJsonObject patchReviewDecisionObject(const CaseManifest& manifest,
                                      const QString& overallStatusWithoutReview,
                                      bool verificationFailed)
{
    const QString state = reviewStatusState(manifest.patchReviewStatus);
    QString gateState = QStringLiteral("unknown");
    QString recommendation = QStringLiteral("capture_or_submit_review");
    QString summary = QStringLiteral("Patch candidate is not yet ready for verification sign-off.");
    QJsonArray blockers;

    if (state == QStringLiteral("approved"))
    {
        gateState = verificationFailed ? QStringLiteral("blocked") : QStringLiteral("passed");
        recommendation = verificationFailed ? QStringLiteral("fix_verification_failures") : QStringLiteral("ready_for_regression_signoff");
        summary = verificationFailed
            ? QStringLiteral("Patch review is approved, but verification still has failing gates.")
            : QStringLiteral("Patch review is approved and verification has no failing gate.");
        if (verificationFailed)
        {
            blockers.append(QStringLiteral("verification_failed"));
        }
    }
    else if (state == QStringLiteral("rejected"))
    {
        gateState = QStringLiteral("failed");
        recommendation = QStringLiteral("revise_patch_candidate");
        summary = QStringLiteral("Patch candidate was rejected by review.");
        blockers.append(QStringLiteral("review_rejected"));
    }
    else if (state == QStringLiteral("needs_review"))
    {
        gateState = QStringLiteral("pending");
        recommendation = QStringLiteral("wait_for_reviewer_decision");
        summary = QStringLiteral("Patch candidate is submitted and waiting for review.");
        blockers.append(QStringLiteral("review_pending"));
    }
    else
    {
        gateState = QStringLiteral("draft");
        recommendation = QStringLiteral("submit_patch_for_review");
        summary = QStringLiteral("Patch candidate is still in draft review state.");
        blockers.append(QStringLiteral("review_draft"));
    }

    if (overallStatusWithoutReview == QStringLiteral("incomplete") && state == QStringLiteral("approved"))
    {
        blockers.append(QStringLiteral("verification_incomplete"));
    }

    return QJsonObject {
        {QStringLiteral("review_status"), sanitized(manifest.patchReviewStatus.isEmpty() ? QStringLiteral("Draft") : manifest.patchReviewStatus)},
        {QStringLiteral("state"), state},
        {QStringLiteral("gate_state"), gateState},
        {QStringLiteral("recommendation"), recommendation},
        {QStringLiteral("summary"), summary},
        {QStringLiteral("blockers"), blockers},
        {QStringLiteral("review_items"), patchReviewItemsArray(manifest.patchReviewItems)},
    };
}

TestgridTotals totalsForRows(const QVector<TestgridRow>& rows)
{
    TestgridTotals totals;
    for (const TestgridRow& row : rows)
    {
        totals.run += row.runCount.toInt();
        totals.pass += row.passCount.toInt();
        totals.fail += row.failCount.toInt();
    }
    return totals;
}

QJsonArray testgridRowArray(const QVector<TestgridRow>& rows)
{
    QJsonArray array;
    for (const TestgridRow& row : rows)
    {
        array.append(QJsonObject {
            {QStringLiteral("module"), row.module},
            {QStringLiteral("run_count"), row.runCount},
            {QStringLiteral("pass_count"), row.passCount},
            {QStringLiteral("fail_count"), row.failCount},
            {QStringLiteral("pass_rate"), row.passRate},
        });
    }
    return array;
}

QJsonArray failureDetailsForRows(const QVector<TestgridRow>& rows)
{
    QJsonArray array;
    for (const TestgridRow& row : rows)
    {
        const QString module = row.module.trimmed();
        const QString moduleLower = module.toLower();
        if (moduleLower == QStringLiteral("total")
            || moduleLower == QStringLiteral("summary")
            || row.failCount.toInt() <= 0)
        {
            continue;
        }
        array.append(QJsonObject {
            {QStringLiteral("type"), QStringLiteral("testgrid")},
            {QStringLiteral("name"), module},
            {QStringLiteral("status"), QStringLiteral("failed")},
            {QStringLiteral("summary"), QStringLiteral("%1 failed out of %2").arg(row.failCount, row.runCount)},
        });
    }
    return array;
}

QJsonObject loadJsonObject(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return {};
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    return doc.object();
}

QString firstExistingPatchDryRunArtifact(const QDir& root)
{
    const QStringList candidates {
        QStringLiteral("artifacts/patch_apply_dry_run_result.json"),
        QStringLiteral("artifacts/patch_undo_dry_run_result.json"),
    };
    for (const QString& candidate : candidates)
    {
        if (QFileInfo::exists(root.filePath(candidate)))
        {
            return candidate;
        }
    }
    return QString();
}

QString firstExistingPatchGenerateArtifact(const QDir& root)
{
    const QString candidate = QStringLiteral("artifacts/patch_generate_result.json");
    return QFileInfo::exists(root.filePath(candidate)) ? candidate : QString();
}

QJsonArray sanitizedTestdiffEntries(const QJsonObject& testgridResult)
{
    QJsonArray out;
    const QJsonArray entries = testgridResult.value(QStringLiteral("testdiff_entries")).toArray();
    for (const QJsonValue& value : entries)
    {
        const QJsonObject entry = value.toObject();
        out.append(QJsonObject {
            {QStringLiteral("name"), sanitized(entry.value(QStringLiteral("name")).toString())},
            {QStringLiteral("status"), sanitized(entry.value(QStringLiteral("status")).toString())},
            {QStringLiteral("metric"), sanitized(entry.value(QStringLiteral("metric")).toString())},
            {QStringLiteral("note"), sanitized(entry.value(QStringLiteral("note")).toString())},
        });
    }
    return out;
}

QJsonArray sanitizedFailureDetailsFromResult(const QJsonObject& testgridResult)
{
    QJsonArray out;
    const QJsonArray failures = testgridResult.value(QStringLiteral("failure_details")).toArray();
    for (const QJsonValue& value : failures)
    {
        const QJsonObject failure = value.toObject();
        QJsonObject item {
            {QStringLiteral("type"), sanitized(failure.value(QStringLiteral("type")).toString())},
            {QStringLiteral("name"), sanitized(failure.value(QStringLiteral("name")).toString())},
            {QStringLiteral("status"), sanitized(failure.value(QStringLiteral("status")).toString())},
            {QStringLiteral("summary"), sanitized(failure.value(QStringLiteral("summary")).toString())},
        };
        const QString artifact = normalizedPath(sanitized(failure.value(QStringLiteral("artifact")).toString()));
        if (!artifact.isEmpty())
        {
            item.insert(QStringLiteral("artifact"), artifact);
        }
        out.append(item);
    }
    return out;
}

QJsonObject sanitizedTimingObject(const QJsonObject& testgridResult)
{
    const QJsonObject raw = testgridResult.value(QStringLiteral("timing")).toObject();
    QJsonArray entries;
    for (const QJsonValue& value : raw.value(QStringLiteral("entries")).toArray())
    {
        const QJsonObject entry = value.toObject();
        entries.append(QJsonObject {
            {QStringLiteral("name"), sanitized(entry.value(QStringLiteral("name")).toString())},
            {QStringLiteral("elapsed_ms"), entry.value(QStringLiteral("elapsed_ms")).toDouble()},
            {QStringLiteral("status"), sanitized(entry.value(QStringLiteral("status")).toString())},
        });
    }
    return QJsonObject {
        {QStringLiteral("total_elapsed_ms"), raw.value(QStringLiteral("total_elapsed_ms")).toDouble()},
        {QStringLiteral("summary"), sanitized(raw.value(QStringLiteral("summary")).toString())},
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

QJsonObject sanitizedArtifactIndexObject(const QJsonObject& raw)
{
    if (raw.isEmpty())
    {
        return {};
    }

    QJsonArray supportedKinds;
    for (const QJsonValue& value : raw.value(QStringLiteral("supported_kinds")).toArray())
    {
        const QString kind = sanitized(value.toString());
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
            {QStringLiteral("kind"), sanitized(group.value(QStringLiteral("kind")).toString())},
            {QStringLiteral("key"), sanitized(group.value(QStringLiteral("key")).toString())},
            {QStringLiteral("status"), sanitized(group.value(QStringLiteral("status")).toString())},
        };
        for (const QString& role : {QStringLiteral("before"), QStringLiteral("after"), QStringLiteral("diff")})
        {
            const QString path = normalizedPath(sanitized(group.value(role).toString()));
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

    QJsonObject strategy;
    const QJsonObject rawStrategy = raw.value(QStringLiteral("strategy")).toObject();
    for (auto it = rawStrategy.constBegin(); it != rawStrategy.constEnd(); ++it)
    {
        strategy.insert(sanitized(it.key()), sanitized(it.value().toString()));
    }

    QJsonObject out {
        {QStringLiteral("schema_version"), raw.value(QStringLiteral("schema_version")).toInt()},
        {QStringLiteral("supported_kinds"), supportedKinds},
        {QStringLiteral("counts"), raw.value(QStringLiteral("counts")).toObject()},
        {QStringLiteral("groups"), groups},
        {QStringLiteral("strategy"), strategy},
    };
    out.insert(QStringLiteral("summary"), artifactIndexSummaryObject(out));
    return out;
}

QJsonObject sanitizedTestdiffArtifacts(const QJsonObject& testgridResult)
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
        const QString value = normalizedPath(sanitized(raw.value(key).toString()));
        if (!value.isEmpty())
        {
            out.insert(key, value);
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
            const QString path = normalizedPath(sanitized(item.value(QStringLiteral("path")).toString()));
            if (!path.isEmpty())
            {
                cleanDirectories.append(QJsonObject {
                    {QStringLiteral("role"), sanitized(item.value(QStringLiteral("role")).toString())},
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
            const QString path = normalizedPath(sanitized(item.value(QStringLiteral("path")).toString()));
            if (!path.isEmpty())
            {
                cleanFiles.append(QJsonObject {
                    {QStringLiteral("role"), sanitized(item.value(QStringLiteral("role")).toString())},
                    {QStringLiteral("kind"), sanitized(item.value(QStringLiteral("kind")).toString())},
                    {QStringLiteral("path"), path},
                    {QStringLiteral("bytes"), item.value(QStringLiteral("bytes")).toDouble()},
                });
            }
        }
        out.insert(QStringLiteral("artifact_files"), cleanFiles);
    }
    const QJsonObject artifactIndex = sanitizedArtifactIndexObject(raw.value(QStringLiteral("artifact_index")).toObject());
    if (!artifactIndex.isEmpty())
    {
        out.insert(QStringLiteral("artifact_index"), artifactIndex);
        out.insert(QStringLiteral("artifact_index_summary"), artifactIndex.value(QStringLiteral("summary")).toObject());
    }
    const QJsonObject artifactAnalysis =
        sanitizedJsonValue(raw.value(QStringLiteral("artifact_analysis"))).toObject();
    if (!artifactAnalysis.isEmpty())
    {
        out.insert(QStringLiteral("artifact_analysis"), artifactAnalysis);
    }
    out.insert(QStringLiteral("truncated"), raw.value(QStringLiteral("truncated")).toBool(false));
    return out;
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

QJsonArray comparisonRowArray(const TestgridComparison& comparison)
{
    QJsonArray rows;
    for (const TestgridComparisonRow& row : comparison.rows)
    {
        rows.append(QJsonObject {
            {QStringLiteral("module"), sanitized(row.module)},
            {QStringLiteral("before_run_count"), sanitized(row.beforeRunCount)},
            {QStringLiteral("before_pass_count"), sanitized(row.beforePassCount)},
            {QStringLiteral("before_fail_count"), sanitized(row.beforeFailCount)},
            {QStringLiteral("after_run_count"), sanitized(row.afterRunCount)},
            {QStringLiteral("after_pass_count"), sanitized(row.afterPassCount)},
            {QStringLiteral("after_fail_count"), sanitized(row.afterFailCount)},
            {QStringLiteral("pass_delta"), row.passDelta},
            {QStringLiteral("fail_delta"), row.failDelta},
            {QStringLiteral("status"), row.status},
        });
    }
    return rows;
}

QJsonObject comparisonObject(const TestgridComparison& comparison,
                             const QString& beforeSummary,
                             const QString& afterSummary)
{
    const bool available = comparison.isAvailable();
    return QJsonObject {
        {QStringLiteral("available"), available},
        {QStringLiteral("status"), !available ? QStringLiteral("unavailable") : (comparison.hasRegression() ? QStringLiteral("regressed") : QStringLiteral("not_regressed"))},
        {QStringLiteral("summary"), sanitized(comparison.summaryText())},
        {QStringLiteral("before_run_total"), comparison.beforeRunTotal},
        {QStringLiteral("before_pass_total"), comparison.beforePassTotal},
        {QStringLiteral("before_fail_total"), comparison.beforeFailTotal},
        {QStringLiteral("after_run_total"), comparison.afterRunTotal},
        {QStringLiteral("after_pass_total"), comparison.afterPassTotal},
        {QStringLiteral("after_fail_total"), comparison.afterFailTotal},
        {QStringLiteral("pass_delta"), comparison.passDelta},
        {QStringLiteral("fail_delta"), comparison.failDelta},
        {QStringLiteral("before_summary"), normalizedPath(beforeSummary)},
        {QStringLiteral("after_summary"), normalizedPath(afterSummary)},
        {QStringLiteral("rows"), comparisonRowArray(comparison)},
    };
}

QJsonArray sanitizedComparisonRows(const QJsonArray& rows)
{
    QJsonArray out;
    for (const QJsonValue& value : rows)
    {
        const QJsonObject row = value.toObject();
        out.append(QJsonObject {
            {QStringLiteral("module"), sanitized(row.value(QStringLiteral("module")).toString())},
            {QStringLiteral("before_run_count"), sanitized(row.value(QStringLiteral("before_run_count")).toString())},
            {QStringLiteral("before_pass_count"), sanitized(row.value(QStringLiteral("before_pass_count")).toString())},
            {QStringLiteral("before_fail_count"), sanitized(row.value(QStringLiteral("before_fail_count")).toString())},
            {QStringLiteral("after_run_count"), sanitized(row.value(QStringLiteral("after_run_count")).toString())},
            {QStringLiteral("after_pass_count"), sanitized(row.value(QStringLiteral("after_pass_count")).toString())},
            {QStringLiteral("after_fail_count"), sanitized(row.value(QStringLiteral("after_fail_count")).toString())},
            {QStringLiteral("pass_delta"), row.value(QStringLiteral("pass_delta")).toInt()},
            {QStringLiteral("fail_delta"), row.value(QStringLiteral("fail_delta")).toInt()},
            {QStringLiteral("status"), sanitized(row.value(QStringLiteral("status")).toString())},
        });
    }
    return out;
}

QJsonObject sanitizedBeforeAfterObject(const QJsonObject& raw)
{
    return QJsonObject {
        {QStringLiteral("available"), raw.value(QStringLiteral("available")).toBool()},
        {QStringLiteral("status"), sanitized(raw.value(QStringLiteral("status")).toString(QStringLiteral("unavailable")))},
        {QStringLiteral("summary"), sanitized(raw.value(QStringLiteral("summary")).toString())},
        {QStringLiteral("before_run_total"), raw.value(QStringLiteral("before_run_total")).toInt()},
        {QStringLiteral("before_pass_total"), raw.value(QStringLiteral("before_pass_total")).toInt()},
        {QStringLiteral("before_fail_total"), raw.value(QStringLiteral("before_fail_total")).toInt()},
        {QStringLiteral("after_run_total"), raw.value(QStringLiteral("after_run_total")).toInt()},
        {QStringLiteral("after_pass_total"), raw.value(QStringLiteral("after_pass_total")).toInt()},
        {QStringLiteral("after_fail_total"), raw.value(QStringLiteral("after_fail_total")).toInt()},
        {QStringLiteral("pass_delta"), raw.value(QStringLiteral("pass_delta")).toInt()},
        {QStringLiteral("fail_delta"), raw.value(QStringLiteral("fail_delta")).toInt()},
        {QStringLiteral("before_summary"), normalizedPath(sanitized(raw.value(QStringLiteral("before_summary")).toString()))},
        {QStringLiteral("after_summary"), normalizedPath(sanitized(raw.value(QStringLiteral("after_summary")).toString()))},
        {QStringLiteral("rows"), sanitizedComparisonRows(raw.value(QStringLiteral("rows")).toArray())},
    };
}

QJsonObject sanitizedJsonObject(const QJsonObject& raw)
{
    QJsonObject out;
    for (auto it = raw.constBegin(); it != raw.constEnd(); ++it)
    {
        out.insert(it.key(), sanitizedJsonValue(it.value()));
    }
    return out;
}

QJsonArray sanitizedJsonArray(const QJsonArray& raw)
{
    QJsonArray out;
    for (const QJsonValue& value : raw)
    {
        out.append(sanitizedJsonValue(value));
    }
    return out;
}

QJsonValue sanitizedJsonValue(const QJsonValue& value)
{
    if (value.isString())
    {
        return sanitized(value.toString());
    }
    if (value.isObject())
    {
        return sanitizedJsonObject(value.toObject());
    }
    if (value.isArray())
    {
        return sanitizedJsonArray(value.toArray());
    }
    return value;
}

QJsonObject sanitizedTopologyCompareObject(const QJsonObject& raw)
{
    return sanitizedJsonObject(raw);
}

QJsonObject beforeAfterObject(const CaseManifest& manifest, const QDir& root, const QJsonObject& testgridResult)
{
    const QJsonObject fromResult = testgridResult.value(QStringLiteral("before_after")).toObject();
    if (!fromResult.isEmpty())
    {
        return sanitizedBeforeAfterObject(fromResult);
    }

    const QString beforeRelative = QStringLiteral("verification/testgrid_before.txt");
    const QString afterRelative = QStringLiteral("verification/testgrid_after.txt");
    const QString beforePath = root.filePath(beforeRelative);
    const QString afterPath = root.filePath(afterRelative);
    const QVector<TestgridRow> beforeRows = VerificationResultParser::parseTestgridText(readTextFile(beforePath));
    QVector<TestgridRow> afterRows = VerificationResultParser::parseTestgridText(readTextFile(afterPath));
    if (afterRows.isEmpty())
    {
        afterRows = manifest.testgridRows;
    }
    const TestgridComparison comparison = VerificationResultParser::compareTestgridRows(beforeRows, afterRows);
    return comparisonObject(comparison,
                            QFileInfo::exists(beforePath) ? beforeRelative : QString(),
                            QFileInfo::exists(afterPath) ? afterRelative : QString());
}

QJsonArray comparisonFailureDetails(const QJsonObject& beforeAfter)
{
    QJsonArray out;
    const QJsonArray rows = beforeAfter.value(QStringLiteral("rows")).toArray();
    for (const QJsonValue& value : rows)
    {
        const QJsonObject row = value.toObject();
        if (row.value(QStringLiteral("fail_delta")).toInt() <= 0)
        {
            continue;
        }
        out.append(QJsonObject {
            {QStringLiteral("type"), QStringLiteral("before_after")},
            {QStringLiteral("name"), row.value(QStringLiteral("module")).toString()},
            {QStringLiteral("status"), row.value(QStringLiteral("status")).toString(QStringLiteral("regressed"))},
            {QStringLiteral("summary"), QStringLiteral("fail %1 -> %2 (delta %3)")
                .arg(row.value(QStringLiteral("before_fail_count")).toString())
                .arg(row.value(QStringLiteral("after_fail_count")).toString())
                .arg(row.value(QStringLiteral("fail_delta")).toInt())},
        });
    }
    return out;
}

QJsonArray patchConflictArray(const QJsonObject& patchDryRunResult)
{
    QJsonArray out;
    const QJsonArray conflicts = patchDryRunResult.value(QStringLiteral("conflicts")).toArray();
    for (const QJsonValue& value : conflicts)
    {
        const QJsonObject conflict = value.toObject();
        out.append(QJsonObject {
            {QStringLiteral("message"), sanitized(conflict.value(QStringLiteral("message")).toString())},
            {QStringLiteral("source_file"), sanitized(conflict.value(QStringLiteral("source_file")).toString())},
            {QStringLiteral("source_line"), conflict.value(QStringLiteral("source_line")).toInt()},
        });
    }
    return out;
}

QString gateState(bool known, bool passed)
{
    if (!known)
    {
        return QStringLiteral("unknown");
    }
    return passed ? QStringLiteral("passed") : QStringLiteral("failed");
}

bool statusContainsFailure(const QString& status)
{
    const QString lowered = status.toLower();
    return lowered.contains(QStringLiteral("failed"))
        || lowered.contains(QStringLiteral("error"))
        || lowered.contains(QStringLiteral("rejected"));
}

QString relativeReportLink(const QString& targetRelativePath, const QString& caseRoot, const QString& markdownPath)
{
    const QString absoluteTarget = QDir(caseRoot).absoluteFilePath(targetRelativePath);
    return normalizedPath(QDir(QFileInfo(markdownPath).absolutePath()).relativeFilePath(absoluteTarget));
}

bool writeJson(const QJsonObject& report, const QString& jsonPath, QString* error)
{
    const QFileInfo outputInfo(jsonPath);
    QDir dir;
    if (!dir.mkpath(outputInfo.absolutePath()))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot create verification json directory: %1").arg(outputInfo.absolutePath());
        }
        return false;
    }

    QSaveFile file(jsonPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot write verification json %1: %2").arg(jsonPath, file.errorString());
        }
        return false;
    }
    file.write(QJsonDocument(report).toJson(QJsonDocument::Indented));
    if (!file.commit())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot commit verification json %1: %2").arg(jsonPath, file.errorString());
        }
        return false;
    }
    return true;
}

bool writeMarkdown(const CaseManifest& manifest,
                   const QJsonObject& report,
                   const QString& caseRoot,
                   const QString& markdownPath,
                   const QString& jsonPath,
                   QString* error)
{
    const QFileInfo outputInfo(markdownPath);
    QDir dir;
    if (!dir.mkpath(outputInfo.absolutePath()))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot create verification report directory: %1").arg(outputInfo.absolutePath());
        }
        return false;
    }

    QSaveFile file(markdownPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot write verification report %1: %2").arg(markdownPath, file.errorString());
        }
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    const QJsonObject gate = report.value(QStringLiteral("gate")).toObject();
    const QJsonObject testgrid = report.value(QStringLiteral("testgrid")).toObject();
    const QJsonObject artifacts = report.value(QStringLiteral("artifacts")).toObject();
    const QJsonObject beforeAfter = report.value(QStringLiteral("before_after")).toObject();
    const QJsonObject geometryDiff = report.value(QStringLiteral("geometry_diff")).toObject();
    const QJsonObject timing = report.value(QStringLiteral("timing")).toObject();

    out << "# Verification Report\n\n";
    out << "- Case ID: `" << manifest.caseId << "`\n";
    out << "- Title: " << sanitized(manifest.title) << "\n";
    out << "- Status: " << sanitized(manifest.status) << "\n";
    out << "- Generated at: " << report.value(QStringLiteral("generated_at")).toString() << "\n";
    out << "- Overall gate: `" << report.value(QStringLiteral("overall_status")).toString() << "`\n";
    out << "- Scope: `" << report.value(QStringLiteral("scope")).toString() << "`\n\n";

    out << "## Gate Decision\n\n";
    out << "| Gate | State | Note |\n";
    out << "|---|---|---|\n";
    out << "| draw_smoke | " << gate.value(QStringLiteral("draw_smoke")).toObject().value(QStringLiteral("state")).toString()
        << " | " << escapedCell(gate.value(QStringLiteral("draw_smoke")).toObject().value(QStringLiteral("note")).toString()) << " |\n";
    out << "| testgrid | " << gate.value(QStringLiteral("testgrid")).toObject().value(QStringLiteral("state")).toString()
        << " | " << escapedCell(gate.value(QStringLiteral("testgrid")).toObject().value(QStringLiteral("note")).toString()) << " |\n";
    out << "| testdiff | " << gate.value(QStringLiteral("testdiff")).toObject().value(QStringLiteral("state")).toString()
        << " | " << escapedCell(gate.value(QStringLiteral("testdiff")).toObject().value(QStringLiteral("note")).toString()) << " |\n";
    out << "| before_after | " << gate.value(QStringLiteral("before_after")).toObject().value(QStringLiteral("state")).toString()
        << " | " << escapedCell(gate.value(QStringLiteral("before_after")).toObject().value(QStringLiteral("note")).toString()) << " |\n";
    out << "| patch_apply | " << gate.value(QStringLiteral("patch_apply")).toObject().value(QStringLiteral("state")).toString()
        << " | " << escapedCell(gate.value(QStringLiteral("patch_apply")).toObject().value(QStringLiteral("note")).toString()) << " |\n";
    out << "| patch_dry_run | " << gate.value(QStringLiteral("patch_dry_run")).toObject().value(QStringLiteral("state")).toString()
        << " | " << escapedCell(gate.value(QStringLiteral("patch_dry_run")).toObject().value(QStringLiteral("note")).toString()) << " |\n";
    out << "| patch_review | " << gate.value(QStringLiteral("patch_review")).toObject().value(QStringLiteral("state")).toString()
        << " | " << escapedCell(gate.value(QStringLiteral("patch_review")).toObject().value(QStringLiteral("note")).toString()) << " |\n";
    out << "| patch_signoff | " << gate.value(QStringLiteral("patch_signoff")).toObject().value(QStringLiteral("state")).toString()
        << " | " << escapedCell(gate.value(QStringLiteral("patch_signoff")).toObject().value(QStringLiteral("note")).toString()) << " |\n\n";

    out << "## Verification Items\n\n";
    out << "| Item | Value |\n";
    out << "|---|---|\n";
    for (const LabelValue& item : manifest.verificationItems)
    {
        out << "| " << escapedCell(item.label) << " | " << escapedCell(item.value) << " |\n";
    }
    out << "\n";

    out << "## Timing Summary\n\n";
    out << "- Total elapsed: " << timing.value(QStringLiteral("total_elapsed_ms")).toDouble() << " ms\n\n";
    out << "| Step | Elapsed ms | Status |\n";
    out << "|---|---:|---|\n";
    const QJsonArray timingEntries = timing.value(QStringLiteral("entries")).toArray();
    if (timingEntries.isEmpty())
    {
        out << "| _none_ | 0 | unavailable |\n";
    }
    for (const QJsonValue& value : timingEntries)
    {
        const QJsonObject entry = value.toObject();
        out << "| " << escapedCell(entry.value(QStringLiteral("name")).toString())
            << " | " << entry.value(QStringLiteral("elapsed_ms")).toDouble()
            << " | " << escapedCell(entry.value(QStringLiteral("status")).toString()) << " |\n";
    }
    out << "\n";

    out << "## Testgrid Summary\n\n";
    out << "- Run: " << testgrid.value(QStringLiteral("run_total")).toInt() << "\n";
    out << "- Passed: " << testgrid.value(QStringLiteral("pass_total")).toInt() << "\n";
    out << "- Failed: " << testgrid.value(QStringLiteral("fail_total")).toInt() << "\n\n";
    out << "| Module | Run | Passed | Failed | Pass Rate |\n";
    out << "|---|---:|---:|---:|---:|\n";
    for (const TestgridRow& row : manifest.testgridRows)
    {
        out << "| " << escapedCell(row.module)
            << " | " << escapedCell(row.runCount)
            << " | " << escapedCell(row.passCount)
            << " | " << escapedCell(row.failCount)
            << " | " << escapedCell(row.passRate) << " |\n";
    }
    if (manifest.testgridRows.isEmpty())
    {
        out << "| _none_ | 0 | 0 | 0 | - |\n";
    }
    out << "\n";

    const QJsonObject testdiff = report.value(QStringLiteral("testdiff")).toObject();
    const QJsonObject testdiffArtifacts = testdiff.value(QStringLiteral("artifacts")).toObject();
    const QJsonObject artifactIndex =
        testdiffArtifacts.value(QStringLiteral("artifact_index")).toObject();
    const QJsonObject artifactIndexSummary =
        testdiffArtifacts.value(QStringLiteral("artifact_index_summary")).toObject();

    out << "## Testdiff Artifact Index\n\n";
    out << "- Groups: " << artifactIndexSummary.value(QStringLiteral("total_groups")).toInt() << "\n";
    const QJsonObject statusCounts = artifactIndexSummary.value(QStringLiteral("status_counts")).toObject();
    out << "- Paired with diff: " << statusCounts.value(QStringLiteral("paired_with_diff")).toInt()
        << ", paired: " << statusCounts.value(QStringLiteral("paired")).toInt()
        << ", diff only: " << statusCounts.value(QStringLiteral("diff_only")).toInt()
        << ", incomplete: " << statusCounts.value(QStringLiteral("incomplete")).toInt() << "\n\n";
    out << "| Kind | Before | After | Diff | Total | Groups |\n";
    out << "|---|---:|---:|---:|---:|---:|\n";
    const QJsonObject kindGroupCounts = artifactIndexSummary.value(QStringLiteral("kind_group_counts")).toObject();
    const QStringList indexedKinds {
        QStringLiteral("image"),
        QStringLiteral("property"),
        QStringLiteral("performance"),
    };
    bool wroteKindRow = false;
    for (const QString& kind : indexedKinds)
    {
        const QJsonObject counts = artifactIndex.value(QStringLiteral("counts")).toObject().value(kind).toObject();
        const int total = counts.value(QStringLiteral("total")).toInt();
        const int groups = kindGroupCounts.value(kind).toInt();
        if (total == 0 && groups == 0)
        {
            continue;
        }
        wroteKindRow = true;
        out << "| " << escapedCell(kind)
            << " | " << counts.value(QStringLiteral("before")).toInt()
            << " | " << counts.value(QStringLiteral("after")).toInt()
            << " | " << counts.value(QStringLiteral("diff")).toInt()
            << " | " << total
            << " | " << groups << " |\n";
    }
    if (!wroteKindRow)
    {
        out << "| _none_ | 0 | 0 | 0 | 0 | 0 |\n";
    }
    out << "\n";
    out << "| Kind | Key | Status | Before | After | Diff |\n";
    out << "|---|---|---|---|---|---|\n";
    const QJsonArray indexGroups = artifactIndex.value(QStringLiteral("groups")).toArray();
    const int maxArtifactIndexGroups = 12;
    if (indexGroups.isEmpty())
    {
        out << "| _none_ | _none_ | unavailable | - | - | - |\n";
    }
    for (int index = 0; index < indexGroups.size() && index < maxArtifactIndexGroups; ++index)
    {
        const QJsonObject group = indexGroups.at(index).toObject();
        out << "| " << escapedCell(group.value(QStringLiteral("kind")).toString())
            << " | " << escapedCell(group.value(QStringLiteral("key")).toString())
            << " | " << escapedCell(group.value(QStringLiteral("status")).toString())
            << " | " << escapedCell(group.value(QStringLiteral("before")).toString(QStringLiteral("-")))
            << " | " << escapedCell(group.value(QStringLiteral("after")).toString(QStringLiteral("-")))
            << " | " << escapedCell(group.value(QStringLiteral("diff")).toString(QStringLiteral("-"))) << " |\n";
    }
    if (indexGroups.size() > maxArtifactIndexGroups)
    {
        out << "| _truncated_ | " << indexGroups.size()
            << " groups total | see JSON | - | - | - |\n";
    }
    out << "\n";

    const QJsonObject artifactAnalysis =
        testdiffArtifacts.value(QStringLiteral("artifact_analysis")).toObject();
    const QJsonObject artifactAnalysisSummary =
        artifactAnalysis.value(QStringLiteral("summary")).toObject();
    out << "## Testdiff Artifact Analysis\n\n";
    out << "- Analyzed groups: " << artifactAnalysisSummary.value(QStringLiteral("groups")).toInt() << "\n";
    out << "- Image diffs supplied by runner: "
        << artifactAnalysisSummary.value(QStringLiteral("image_diff_supplied_by_runner")).toInt() << "\n";
    out << "- Property JSON parsed: "
        << artifactAnalysisSummary.value(QStringLiteral("property_json_parsed")).toInt() << "\n";
    out << "- Performance metrics extracted: "
        << artifactAnalysisSummary.value(QStringLiteral("performance_metrics")).toInt() << "\n\n";
    out << "| Kind | Key | Status | Parsed Detail |\n";
    out << "|---|---|---|---|\n";
    const QJsonArray analysisGroups = artifactAnalysis.value(QStringLiteral("groups")).toArray();
    if (analysisGroups.isEmpty())
    {
        out << "| _none_ | _none_ | unavailable | - |\n";
    }
    for (const QJsonValue& value : analysisGroups)
    {
        const QJsonObject group = value.toObject();
        const QString kind = group.value(QStringLiteral("kind")).toString();
        const QJsonObject analysis = group.value(QStringLiteral("analysis")).toObject();
        QString detail = QStringLiteral("-");
        if (kind == QStringLiteral("image"))
        {
            detail = analysis.value(QStringLiteral("diff_supplied_by_runner")).toBool()
                ? QStringLiteral("runner diff artifact supplied")
                : QStringLiteral("no diff artifact supplied");
        }
        else if (kind == QStringLiteral("property"))
        {
            const QJsonObject json = analysis.value(QStringLiteral("json")).toObject();
            detail = json.value(QStringLiteral("valid_json")).toBool()
                ? QStringLiteral("json %1 keys").arg(json.value(QStringLiteral("top_level_key_count")).toInt())
                : QStringLiteral("json unavailable");
        }
        else if (kind == QStringLiteral("performance"))
        {
            detail = QStringLiteral("%1 metric(s)")
                .arg(analysis.value(QStringLiteral("metrics")).toArray().size());
        }
        out << "| " << escapedCell(kind)
            << " | " << escapedCell(group.value(QStringLiteral("key")).toString())
            << " | " << escapedCell(group.value(QStringLiteral("status")).toString())
            << " | " << escapedCell(detail) << " |\n";
    }
    out << "\n";

    out << "## Before / After Comparison\n\n";
    out << "- Status: `" << beforeAfter.value(QStringLiteral("status")).toString(QStringLiteral("unavailable")) << "`\n";
    out << "- Summary: " << sanitized(beforeAfter.value(QStringLiteral("summary")).toString(QStringLiteral("before/after comparison unavailable"))) << "\n";
    const QString beforeSummary = beforeAfter.value(QStringLiteral("before_summary")).toString();
    const QString afterSummary = beforeAfter.value(QStringLiteral("after_summary")).toString();
    if (!beforeSummary.isEmpty())
    {
        out << "- Before summary: `" << sanitized(beforeSummary) << "`\n";
    }
    if (!afterSummary.isEmpty())
    {
        out << "- After summary: `" << sanitized(afterSummary) << "`\n";
    }
    out << "\n";
    out << "| Module | Before Fail | After Fail | Delta | Status |\n";
    out << "|---|---:|---:|---:|---|\n";
    const QJsonArray comparisonRows = beforeAfter.value(QStringLiteral("rows")).toArray();
    if (comparisonRows.isEmpty())
    {
        out << "| _none_ | 0 | 0 | 0 | unavailable |\n";
    }
    for (const QJsonValue& value : comparisonRows)
    {
        const QJsonObject row = value.toObject();
        out << "| " << escapedCell(row.value(QStringLiteral("module")).toString())
            << " | " << escapedCell(row.value(QStringLiteral("before_fail_count")).toString())
            << " | " << escapedCell(row.value(QStringLiteral("after_fail_count")).toString())
            << " | " << row.value(QStringLiteral("fail_delta")).toInt()
            << " | " << escapedCell(row.value(QStringLiteral("status")).toString()) << " |\n";
    }
    out << "\n";

    out << "## Geometry Diff\n\n";
    out << "- Status: `" << geometryDiff.value(QStringLiteral("status")).toString(QStringLiteral("unavailable")) << "`\n";
    out << "- Summary: " << sanitized(geometryDiff.value(QStringLiteral("summary_text")).toString(QStringLiteral("topology comparison unavailable"))) << "\n";
    const QString topologyArtifact = geometryDiff.value(QStringLiteral("artifact")).toString();
    if (!topologyArtifact.isEmpty())
    {
        out << "- Artifact: `" << sanitized(topologyArtifact) << "`\n";
    }
    const QJsonObject topologySummary = geometryDiff.value(QStringLiteral("match_summary")).toObject();
    if (!topologySummary.isEmpty())
    {
        out << "\n";
        out << "| Matched | Exact | Approximate | Unmatched Before | Unmatched After |\n";
        out << "|---:|---:|---:|---:|---:|\n";
        out << "| " << topologySummary.value(QStringLiteral("matched")).toInt()
            << " | " << topologySummary.value(QStringLiteral("exact_hash_matches")).toInt()
            << " | " << topologySummary.value(QStringLiteral("approximate_matches")).toInt()
            << " | " << topologySummary.value(QStringLiteral("unmatched_before")).toInt()
            << " | " << topologySummary.value(QStringLiteral("unmatched_after")).toInt()
            << " |\n";
    }
    out << "\n";

    out << "## Patch State\n\n";
    const QJsonObject patch = report.value(QStringLiteral("patch")).toObject();
    const QJsonObject patchDecision = patch.value(QStringLiteral("decision")).toObject();
    out << "- Review status: " << sanitized(manifest.patchReviewStatus) << "\n";
    out << "- Apply status: " << sanitized(manifest.patchApplyStatus) << "\n";
    out << "- Dry-run status: " << sanitized(patch.value(QStringLiteral("dry_run_status")).toString()) << "\n";
    out << "- Review gate: `" << patchDecision.value(QStringLiteral("gate_state")).toString(QStringLiteral("unknown")) << "`\n";
    out << "- Recommendation: `" << patchDecision.value(QStringLiteral("recommendation")).toString(QStringLiteral("capture_or_submit_review")) << "`\n";
    out << "- Decision summary: " << sanitized(patchDecision.value(QStringLiteral("summary")).toString()) << "\n";
    out << "- Signoff status: " << sanitized(patch.value(QStringLiteral("signoff_status")).toString()) << "\n";
    out << "- Signoff note: " << sanitized(patch.value(QStringLiteral("signoff_note")).toString()) << "\n";
    const QJsonArray patchConflicts = patch.value(QStringLiteral("conflicts")).toArray();
    if (!patchConflicts.isEmpty())
    {
        out << "- Conflict hints: " << patchConflicts.size() << "\n";
    }
    if (!manifest.patchApplyLog.trimmed().isEmpty())
    {
        out << "- Apply log: " << sanitized(manifest.patchApplyLog) << "\n";
    }
    const QJsonArray reviewItems = patchDecision.value(QStringLiteral("review_items")).toArray();
    if (!reviewItems.isEmpty())
    {
        out << "\n";
        out << "| Review Step | State |\n";
        out << "|---|---|\n";
        for (const QJsonValue& value : reviewItems)
        {
            const QJsonObject item = value.toObject();
            out << "| " << escapedCell(item.value(QStringLiteral("step")).toString())
                << " | " << escapedCell(item.value(QStringLiteral("state")).toString()) << " |\n";
        }
    }
    out << "\n";

    out << "## Failure Details\n\n";
    out << "| Type | Name | Status | Summary | Artifact |\n";
    out << "|---|---|---|---|---|\n";
    const QJsonArray failures = report.value(QStringLiteral("failure_details")).toArray();
    if (failures.isEmpty())
    {
        out << "| _none_ | _none_ | ok | No structured failures recorded. | - |\n";
    }
    for (const QJsonValue& value : failures)
    {
        const QJsonObject failure = value.toObject();
        out << "| " << escapedCell(failure.value(QStringLiteral("type")).toString())
            << " | " << escapedCell(failure.value(QStringLiteral("name")).toString())
            << " | " << escapedCell(failure.value(QStringLiteral("status")).toString())
            << " | " << escapedCell(failure.value(QStringLiteral("summary")).toString())
            << " | " << escapedCell(failure.value(QStringLiteral("artifact")).toString(QStringLiteral("-"))) << " |\n";
    }
    out << "\n";

    out << "## Linked Artifacts\n\n";
    const QString evidenceBundlePath = artifacts.value(QStringLiteral("evidence_bundle")).toString();
    if (!evidenceBundlePath.isEmpty())
    {
        out << "- Evidence bundle: [" << evidenceBundlePath << "]("
            << relativeReportLink(evidenceBundlePath, caseRoot, markdownPath) << ")\n";
    }
    const QString testgridResultPath = artifacts.value(QStringLiteral("testgrid_result")).toString();
    if (!testgridResultPath.isEmpty())
    {
        out << "- Testgrid result: [" << testgridResultPath << "]("
            << relativeReportLink(testgridResultPath, caseRoot, markdownPath) << ")\n";
    }
    const QString testdiffSummaryPath = artifacts.value(QStringLiteral("testdiff_summary")).toString();
    if (!testdiffSummaryPath.isEmpty())
    {
        out << "- Testdiff summary: [" << testdiffSummaryPath << "]("
            << relativeReportLink(testdiffSummaryPath, caseRoot, markdownPath) << ")\n";
    }
    const QString testdiffStdoutPath = artifacts.value(QStringLiteral("testdiff_command_stdout")).toString();
    if (!testdiffStdoutPath.isEmpty())
    {
        out << "- Testdiff command stdout: [" << testdiffStdoutPath << "]("
            << relativeReportLink(testdiffStdoutPath, caseRoot, markdownPath) << ")\n";
    }
    const QString testdiffStderrPath = artifacts.value(QStringLiteral("testdiff_command_stderr")).toString();
    if (!testdiffStderrPath.isEmpty())
    {
        out << "- Testdiff command stderr: [" << testdiffStderrPath << "]("
            << relativeReportLink(testdiffStderrPath, caseRoot, markdownPath) << ")\n";
    }
    const QJsonObject testdiffObjectForLinks = report.value(QStringLiteral("testdiff")).toObject();
    const QJsonObject testdiffArtifactsForLinks =
        testdiffObjectForLinks.value(QStringLiteral("artifacts")).toObject();
    const QJsonArray testdiffFiles =
        testdiffArtifactsForLinks.value(QStringLiteral("artifact_files")).toArray();
    const int maxLinkedTestdiffFiles = 12;
    for (int index = 0; index < testdiffFiles.size() && index < maxLinkedTestdiffFiles; ++index)
    {
        const QJsonObject fileObject = testdiffFiles.at(index).toObject();
        const QString path = fileObject.value(QStringLiteral("path")).toString();
        if (path.isEmpty())
        {
            continue;
        }
        out << "- Testdiff " << fileObject.value(QStringLiteral("kind")).toString(QStringLiteral("artifact"))
            << " (" << fileObject.value(QStringLiteral("role")).toString(QStringLiteral("other")) << "): ["
            << path << "](" << relativeReportLink(path, caseRoot, markdownPath) << ")\n";
    }
    if (testdiffFiles.size() > maxLinkedTestdiffFiles)
    {
        out << "- Testdiff artifacts: " << testdiffFiles.size()
            << " files total; see `testdiff_artifacts.artifact_files` in verification JSON.\n";
    }
    const QString topologyComparePath = artifacts.value(QStringLiteral("topology_compare")).toString();
    if (!topologyComparePath.isEmpty())
    {
        out << "- Topology compare: [" << topologyComparePath << "]("
            << relativeReportLink(topologyComparePath, caseRoot, markdownPath) << ")\n";
    }
    const QString patchDryRunPath = artifacts.value(QStringLiteral("patch_dry_run_result")).toString();
    if (!patchDryRunPath.isEmpty())
    {
        out << "- Patch dry-run result: [" << patchDryRunPath << "]("
            << relativeReportLink(patchDryRunPath, caseRoot, markdownPath) << ")\n";
    }
    const QString patchGeneratePath = artifacts.value(QStringLiteral("patch_generate_result")).toString();
    if (!patchGeneratePath.isEmpty())
    {
        out << "- Patch generation result: [" << patchGeneratePath << "]("
            << relativeReportLink(patchGeneratePath, caseRoot, markdownPath) << ")\n";
    }
    out << "- Structured JSON: [verification/verification_report.json]("
        << normalizedPath(QDir(outputInfo.absolutePath()).relativeFilePath(jsonPath)) << ")\n";

    if (!file.commit())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot commit verification report %1: %2").arg(markdownPath, file.errorString());
        }
        return false;
    }
    return true;
}
} // namespace

QJsonObject VerificationReportWriter::buildReport(const CaseManifest& manifest, const QString& caseRoot)
{
    const QDir root(caseRoot);
    const QString testgridResultRelative = QStringLiteral("artifacts/testgrid_result.json");
    const QString testgridResultPath = root.filePath(testgridResultRelative);
    const bool hasTestgridResult = QFileInfo::exists(testgridResultPath);
    const QJsonObject testgridResult = hasTestgridResult ? loadJsonObject(testgridResultPath) : QJsonObject {};

    const QString drawSmokeValue = itemValue(manifest.verificationItems, QStringLiteral("draw_smoke gate")).isEmpty()
        ? itemValue(manifest.verificationItems, QStringLiteral("DRAW smoke"))
        : itemValue(manifest.verificationItems, QStringLiteral("draw_smoke gate"));
    const QString drawSmokeLower = drawSmokeValue.toLower();
    const bool drawGateKnown = !drawSmokeLower.isEmpty();
    const bool drawGatePassed = drawSmokeLower.contains(QStringLiteral("passed"));
    const QString runnerValue = itemValue(manifest.verificationItems, QStringLiteral("testgrid runner"));
    const bool testgridRunnerExecuted = runnerValue.toLower().contains(QStringLiteral("executed"));

    const TestgridTotals totals = totalsForRows(manifest.testgridRows);
    const bool testgridKnown = !manifest.testgridRows.isEmpty();
    const bool testgridPassed = testgridKnown && totals.fail == 0;
    const QJsonArray testdiffEntries = sanitizedTestdiffEntries(testgridResult);
    const QJsonArray resultFailureDetails = sanitizedFailureDetailsFromResult(testgridResult);
    const QJsonObject timing = sanitizedTimingObject(testgridResult);
    const QJsonObject testdiffArtifacts = sanitizedTestdiffArtifacts(testgridResult);
    const bool testdiffKnown = hasTestgridResult && testgridResult.contains(QStringLiteral("testdiff_entries"));
    const QString testdiffState = testdiffKnown
        ? (testdiffEntries.isEmpty() ? QStringLiteral("not_available") : QStringLiteral("available"))
        : QStringLiteral("unknown");
    const QJsonObject beforeAfter = beforeAfterObject(manifest, root, testgridResult);
    const QJsonObject geometryDiff = sanitizedTopologyCompareObject(TopologyCompareArtifact::loadForCase(caseRoot));
    const QString beforeAfterStatus = beforeAfter.value(QStringLiteral("status")).toString(QStringLiteral("unavailable"));
    const bool beforeAfterKnown = beforeAfter.value(QStringLiteral("available")).toBool();
    const bool beforeAfterRegressed = beforeAfterStatus == QStringLiteral("regressed");
    const bool patchFailed = statusContainsFailure(manifest.patchApplyStatus);
    const QString patchDryRunValue = itemValue(manifest.verificationItems, QStringLiteral("patch dry-run"));
    const QString patchDryRunLower = patchDryRunValue.toLower();
    const bool patchDryRunKnown = !patchDryRunLower.isEmpty();
    const bool patchDryRunFailed = patchDryRunLower.contains(QStringLiteral("failed"));
    const bool patchDryRunPassed = patchDryRunLower.contains(QStringLiteral("passed"));

    QString overallStatus = QStringLiteral("incomplete");
    if (drawGateKnown && !drawGatePassed)
    {
        overallStatus = QStringLiteral("failed");
    }
    else if (testgridKnown && !testgridPassed)
    {
        overallStatus = QStringLiteral("failed");
    }
    else if (patchFailed)
    {
        overallStatus = QStringLiteral("failed");
    }
    else if (patchDryRunFailed)
    {
        overallStatus = QStringLiteral("failed");
    }
    else if (beforeAfterRegressed)
    {
        overallStatus = QStringLiteral("failed");
    }
    else if (drawGateKnown && drawGatePassed && testgridKnown)
    {
        overallStatus = testgridPassed ? QStringLiteral("passed") : QStringLiteral("failed");
    }
    const QString statusBeforeReview = overallStatus;
    const bool verificationFailed = overallStatus == QStringLiteral("failed");
    const QJsonObject patchDecision = patchReviewDecisionObject(manifest, statusBeforeReview, verificationFailed);
    const QString patchReviewGateState = patchDecision.value(QStringLiteral("gate_state")).toString(QStringLiteral("unknown"));
    const QString signoffStatusLower = manifest.patchSignoffStatus.trimmed().toLower();
    const bool signoffSigned = signoffStatusLower == QStringLiteral("signed off");
    const bool signoffBlocked = signoffStatusLower.contains(QStringLiteral("blocked"))
        || signoffStatusLower.contains(QStringLiteral("failed"))
        || signoffStatusLower.contains(QStringLiteral("rejected"));
    const QString signoffGateState = signoffSigned
        ? QStringLiteral("passed")
        : (signoffBlocked ? QStringLiteral("failed") : QStringLiteral("unknown"));
    if (patchReviewGateState == QStringLiteral("failed"))
    {
        overallStatus = QStringLiteral("failed");
    }
    else if (signoffBlocked)
    {
        overallStatus = QStringLiteral("failed");
    }
    else if (patchReviewGateState == QStringLiteral("pending")
        || patchReviewGateState == QStringLiteral("draft")
        || patchReviewGateState == QStringLiteral("unknown")
        || signoffGateState == QStringLiteral("unknown"))
    {
        if (overallStatus == QStringLiteral("passed"))
        {
            overallStatus = QStringLiteral("incomplete");
        }
    }

    const QString scope = testgridRunnerExecuted ? QStringLiteral("draw_smoke_and_testgrid") : QStringLiteral("draw_smoke_only");
    const QJsonObject gate {
        {QStringLiteral("draw_smoke"), QJsonObject {
             {QStringLiteral("state"), gateState(drawGateKnown, drawGatePassed)},
             {QStringLiteral("note"), drawSmokeValue.isEmpty() ? QStringLiteral("DRAW smoke gate has not run") : sanitized(drawSmokeValue)},
         }},
        {QStringLiteral("testgrid"), QJsonObject {
             {QStringLiteral("state"), testgridKnown ? (testgridPassed ? QStringLiteral("passed") : QStringLiteral("failed")) : QStringLiteral("unknown")},
             {QStringLiteral("note"), QStringLiteral("%1 / %2 passed, %3 failed").arg(totals.pass).arg(totals.run).arg(totals.fail)},
         }},
        {QStringLiteral("testdiff"), QJsonObject {
             {QStringLiteral("state"), testdiffState},
             {QStringLiteral("note"), sanitized(itemValue(manifest.verificationItems, QStringLiteral("testdiff")))},
         }},
        {QStringLiteral("before_after"), QJsonObject {
             {QStringLiteral("state"), beforeAfterKnown ? (beforeAfterRegressed ? QStringLiteral("failed") : QStringLiteral("passed")) : QStringLiteral("unknown")},
             {QStringLiteral("note"), beforeAfter.value(QStringLiteral("summary")).toString(QStringLiteral("before/after comparison unavailable"))},
         }},
        {QStringLiteral("patch_apply"), QJsonObject {
             {QStringLiteral("state"), patchFailed ? QStringLiteral("failed") : QStringLiteral("not_failed")},
             {QStringLiteral("note"), sanitized(manifest.patchApplyStatus.isEmpty() ? QStringLiteral("not run") : manifest.patchApplyStatus)},
         }},
        {QStringLiteral("patch_dry_run"), QJsonObject {
             {QStringLiteral("state"), patchDryRunKnown ? (patchDryRunPassed ? QStringLiteral("passed") : (patchDryRunFailed ? QStringLiteral("failed") : QStringLiteral("unknown"))) : QStringLiteral("unknown")},
             {QStringLiteral("note"), sanitized(patchDryRunValue.isEmpty() ? QStringLiteral("not run") : patchDryRunValue)},
         }},
        {QStringLiteral("patch_review"), QJsonObject {
             {QStringLiteral("state"), patchReviewGateState},
             {QStringLiteral("note"), patchDecision.value(QStringLiteral("summary")).toString()},
         }},
        {QStringLiteral("patch_signoff"), QJsonObject {
             {QStringLiteral("state"), signoffGateState},
             {QStringLiteral("note"), sanitized(manifest.patchSignoffStatus.isEmpty()
                    ? QStringLiteral("not requested")
                    : QStringLiteral("%1: %2").arg(manifest.patchSignoffStatus, manifest.patchSignoffNote))},
         }},
    };

    QJsonObject artifacts {
        {QStringLiteral("verification_report"), QStringLiteral("report/verification_report.md")},
        {QStringLiteral("verification_report_json"), QStringLiteral("verification/verification_report.json")},
    };
    if (QFileInfo::exists(root.filePath(QStringLiteral("artifacts/evidence_bundle.json"))))
    {
        artifacts.insert(QStringLiteral("evidence_bundle"), QStringLiteral("artifacts/evidence_bundle.json"));
    }
    if (hasTestgridResult)
    {
        artifacts.insert(QStringLiteral("testgrid_result"), testgridResultRelative);
    }
    const QString testdiffSummaryArtifact = testdiffArtifacts.value(QStringLiteral("summary")).toString();
    if (!testdiffSummaryArtifact.isEmpty())
    {
        artifacts.insert(QStringLiteral("testdiff_summary"), testdiffSummaryArtifact);
    }
    const QString testdiffStdoutArtifact = testdiffArtifacts.value(QStringLiteral("command_stdout")).toString();
    if (!testdiffStdoutArtifact.isEmpty())
    {
        artifacts.insert(QStringLiteral("testdiff_command_stdout"), testdiffStdoutArtifact);
    }
    const QString testdiffStderrArtifact = testdiffArtifacts.value(QStringLiteral("command_stderr")).toString();
    if (!testdiffStderrArtifact.isEmpty())
    {
        artifacts.insert(QStringLiteral("testdiff_command_stderr"), testdiffStderrArtifact);
    }
    const QString patchDryRunArtifact = firstExistingPatchDryRunArtifact(root);
    const QString patchGenerateArtifact = firstExistingPatchGenerateArtifact(root);
    const QJsonObject patchDryRunResult = patchDryRunArtifact.isEmpty()
        ? QJsonObject {}
        : loadJsonObject(root.filePath(patchDryRunArtifact));
    const QJsonArray patchConflicts = patchConflictArray(patchDryRunResult);
    if (!patchDryRunArtifact.isEmpty())
    {
        artifacts.insert(QStringLiteral("patch_dry_run_result"), patchDryRunArtifact);
    }
    if (!patchGenerateArtifact.isEmpty())
    {
        artifacts.insert(QStringLiteral("patch_generate_result"), patchGenerateArtifact);
    }
    const QString topologyCompareArtifact = geometryDiff.value(QStringLiteral("artifact")).toString();
    if (!topologyCompareArtifact.isEmpty())
    {
        artifacts.insert(QStringLiteral("topology_compare"), topologyCompareArtifact);
    }

    QJsonArray failureDetails = resultFailureDetails.isEmpty()
        ? failureDetailsForRows(manifest.testgridRows)
        : resultFailureDetails;
    if (resultFailureDetails.isEmpty())
    {
        const QJsonArray comparisonFailures = comparisonFailureDetails(beforeAfter);
        for (const QJsonValue& value : comparisonFailures)
        {
            failureDetails.append(value);
        }
    }
    if (patchFailed || patchDryRunFailed)
    {
        failureDetails.append(QJsonObject {
            {QStringLiteral("type"), patchDryRunFailed ? QStringLiteral("patch_dry_run") : QStringLiteral("patch_apply")},
            {QStringLiteral("name"), QStringLiteral("candidate_patch")},
            {QStringLiteral("status"), QStringLiteral("failed")},
            {QStringLiteral("summary"), sanitized(patchDryRunFailed ? patchDryRunValue : manifest.patchApplyStatus)},
            {QStringLiteral("artifact"), patchDryRunFailed && !patchDryRunArtifact.isEmpty() ? patchDryRunArtifact : sanitized(manifest.patchApplyLog)},
        });
    }
    if (patchReviewGateState == QStringLiteral("failed") || patchReviewGateState == QStringLiteral("blocked"))
    {
        failureDetails.append(QJsonObject {
            {QStringLiteral("type"), QStringLiteral("patch_review")},
            {QStringLiteral("name"), QStringLiteral("candidate_patch")},
            {QStringLiteral("status"), patchReviewGateState},
            {QStringLiteral("summary"), patchDecision.value(QStringLiteral("summary")).toString()},
        });
    }
    if (signoffBlocked)
    {
        failureDetails.append(QJsonObject {
            {QStringLiteral("type"), QStringLiteral("patch_signoff")},
            {QStringLiteral("name"), QStringLiteral("candidate_patch")},
            {QStringLiteral("status"), QStringLiteral("blocked")},
            {QStringLiteral("summary"), sanitized(manifest.patchSignoffNote.isEmpty() ? manifest.patchSignoffStatus : manifest.patchSignoffNote)},
        });
    }

    return QJsonObject {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("generated_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {QStringLiteral("case_id"), manifest.caseId},
        {QStringLiteral("title"), sanitized(manifest.title)},
        {QStringLiteral("status"), sanitized(manifest.status)},
        {QStringLiteral("overall_status"), overallStatus},
        {QStringLiteral("scope"), scope},
        {QStringLiteral("gate"), gate},
        {QStringLiteral("artifacts"), artifacts},
        {QStringLiteral("verification_items"), labelValueArray(manifest.verificationItems)},
        {QStringLiteral("testgrid"), QJsonObject {
             {QStringLiteral("run_total"), totals.run},
             {QStringLiteral("pass_total"), totals.pass},
             {QStringLiteral("fail_total"), totals.fail},
             {QStringLiteral("rows"), testgridRowArray(manifest.testgridRows)},
         }},
        {QStringLiteral("testdiff"), QJsonObject {
             {QStringLiteral("entries"), testdiffEntries},
             {QStringLiteral("artifacts"), testdiffArtifacts},
             {QStringLiteral("summary"), sanitized(manifest.diffSummary)},
         }},
        {QStringLiteral("timing"), timing},
        {QStringLiteral("before_after"), beforeAfter},
        {QStringLiteral("geometry_diff"), geometryDiff},
        {QStringLiteral("patch"), QJsonObject {
             {QStringLiteral("review_status"), sanitized(manifest.patchReviewStatus)},
             {QStringLiteral("generation_status"), sanitized(itemValue(manifest.verificationItems, QStringLiteral("patch generation")))},
             {QStringLiteral("apply_status"), sanitized(manifest.patchApplyStatus)},
             {QStringLiteral("dry_run_status"), sanitized(patchDryRunValue)},
             {QStringLiteral("apply_log"), sanitized(manifest.patchApplyLog)},
             {QStringLiteral("signoff_status"), sanitized(manifest.patchSignoffStatus)},
             {QStringLiteral("signoff_note"), sanitized(manifest.patchSignoffNote)},
             {QStringLiteral("conflicts"), patchConflicts},
             {QStringLiteral("decision"), patchDecision},
         }},
        {QStringLiteral("failure_details"), failureDetails},
    };
}

bool VerificationReportWriter::writeReport(const CaseManifest& manifest,
                                           const QString& caseRoot,
                                           const QString& markdownPath,
                                           const QString& jsonPath,
                                           QString* error)
{
    const QJsonObject report = buildReport(manifest, caseRoot);
    if (!writeJson(report, jsonPath, error))
    {
        return false;
    }
    return writeMarkdown(manifest, report, caseRoot, markdownPath, jsonPath, error);
}
} // namespace occtdebug
