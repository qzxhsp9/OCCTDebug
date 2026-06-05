#include "core/verify/VerificationResultParser.h"

#include <QMap>
#include <QSet>
#include <QRegularExpression>
#include <QStringList>

namespace occtdebug
{
namespace
{
QStringList splitColumns(const QString& line)
{
    const QString trimmed = line.trimmed();
    if (trimmed.contains(QLatin1Char(',')))
    {
        return trimmed.split(QLatin1Char(','), Qt::SkipEmptyParts);
    }
    if (trimmed.contains(QLatin1Char('|')))
    {
        return trimmed.split(QLatin1Char('|'), Qt::SkipEmptyParts);
    }
    if (trimmed.contains(QLatin1Char('\t')))
    {
        return trimmed.split(QLatin1Char('\t'), Qt::SkipEmptyParts);
    }

    return trimmed.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
}

QString atOrEmpty(const QStringList& values, int index)
{
    return index >= 0 && index < values.size() ? values[index].trimmed() : QString();
}

int intValue(const QString& value)
{
    bool ok = false;
    const int out = value.toInt(&ok);
    return ok ? out : 0;
}

bool isHeaderOrComment(const QString& line)
{
    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty() || trimmed.startsWith(QLatin1Char('#')))
    {
        return true;
    }

    const QString lower = trimmed.toLower();
    return lower.startsWith(QStringLiteral("module "))
        || lower.startsWith(QStringLiteral("module,"))
        || lower.startsWith(QStringLiteral("name "))
        || lower.startsWith(QStringLiteral("name,"));
}

bool isAggregateModule(const QString& module)
{
    const QString lower = module.trimmed().toLower();
    return lower == QStringLiteral("total")
        || lower == QString::fromUtf8("总计")
        || lower == QStringLiteral("summary");
}

QString passRate(const QString& runCount, const QString& passCount, const QString& fallback)
{
    if (!fallback.isEmpty())
    {
        return fallback;
    }

    bool runOk = false;
    bool passOk = false;
    const int run = runCount.toInt(&runOk);
    const int pass = passCount.toInt(&passOk);
    if (!runOk || !passOk || run <= 0)
    {
        return QString();
    }

    return QStringLiteral("%1%").arg(QString::number(static_cast<double>(pass) * 100.0 / run, 'f', 1));
}
} // namespace

QString TestdiffSummary::summaryText() const
{
    return QStringLiteral("testdiff entries=%1 changed=%2 failed=%3")
        .arg(entries.size())
        .arg(changedCount)
        .arg(failedCount);
}

bool TestgridComparison::isAvailable() const
{
    return !rows.isEmpty();
}

bool TestgridComparison::hasRegression() const
{
    return failDelta > 0;
}

QString TestgridComparison::summaryText() const
{
    if (!isAvailable())
    {
        return QStringLiteral("before/after comparison unavailable");
    }
    return QStringLiteral("before/after run %1->%2 pass %3->%4 fail %5->%6 delta_fail=%7")
        .arg(beforeRunTotal)
        .arg(afterRunTotal)
        .arg(beforePassTotal)
        .arg(afterPassTotal)
        .arg(beforeFailTotal)
        .arg(afterFailTotal)
        .arg(failDelta);
}

QString VerificationTimingSummary::summaryText() const
{
    return QStringLiteral("timing entries=%1 total_elapsed_ms=%2")
        .arg(entries.size())
        .arg(totalElapsedMs);
}

QVector<TestgridRow> VerificationResultParser::parseTestgridText(const QString& text)
{
    QVector<TestgridRow> rows;
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("\\r?\\n")));
    rows.reserve(lines.size());

    for (const QString& line : lines)
    {
        if (isHeaderOrComment(line))
        {
            continue;
        }

        const QStringList columns = splitColumns(line);
        if (columns.size() < 4)
        {
            continue;
        }

        TestgridRow row;
        row.module = atOrEmpty(columns, 0);
        row.runCount = atOrEmpty(columns, 1);
        row.passCount = atOrEmpty(columns, 2);
        row.failCount = atOrEmpty(columns, 3);
        row.passRate = passRate(row.runCount, row.passCount, atOrEmpty(columns, 4));
        rows.push_back(row);
    }

    return rows;
}

TestdiffSummary VerificationResultParser::parseTestdiffText(const QString& text)
{
    TestdiffSummary summary;
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("\\r?\\n")));
    summary.entries.reserve(lines.size());

    for (const QString& line : lines)
    {
        if (isHeaderOrComment(line))
        {
            continue;
        }

        const QStringList columns = splitColumns(line);
        if (columns.size() < 2)
        {
            continue;
        }

        TestdiffEntry entry;
        entry.name = atOrEmpty(columns, 0);
        entry.status = atOrEmpty(columns, 1);
        entry.metric = atOrEmpty(columns, 2);
        entry.note = columns.mid(3).join(QLatin1Char(' ')).trimmed();
        summary.entries.push_back(entry);

        const QString status = entry.status.toLower();
        if (status.contains(QStringLiteral("fail")) || status.contains(QStringLiteral("error")))
        {
            ++summary.failedCount;
        }
        if (status.contains(QStringLiteral("diff")) || status.contains(QStringLiteral("changed")))
        {
            ++summary.changedCount;
        }
    }

    return summary;
}

TestgridComparison VerificationResultParser::compareTestgridRows(const QVector<TestgridRow>& beforeRows,
                                                                  const QVector<TestgridRow>& afterRows)
{
    TestgridComparison comparison;
    if (beforeRows.isEmpty() || afterRows.isEmpty())
    {
        return comparison;
    }

    QMap<QString, TestgridRow> beforeByModule;
    QMap<QString, TestgridRow> afterByModule;
    QStringList order;
    QSet<QString> seen;

    const auto addOrder = [&order, &seen](const QString& module) {
        const QString key = module.trimmed();
        if (!key.isEmpty() && !seen.contains(key))
        {
            seen.insert(key);
            order.push_back(key);
        }
    };

    for (const TestgridRow& row : beforeRows)
    {
        const QString key = row.module.trimmed();
        if (key.isEmpty())
        {
            continue;
        }
        beforeByModule.insert(key, row);
        addOrder(key);
        if (!isAggregateModule(key))
        {
            comparison.beforeRunTotal += intValue(row.runCount);
            comparison.beforePassTotal += intValue(row.passCount);
            comparison.beforeFailTotal += intValue(row.failCount);
        }
    }
    for (const TestgridRow& row : afterRows)
    {
        const QString key = row.module.trimmed();
        if (key.isEmpty())
        {
            continue;
        }
        afterByModule.insert(key, row);
        addOrder(key);
        if (!isAggregateModule(key))
        {
            comparison.afterRunTotal += intValue(row.runCount);
            comparison.afterPassTotal += intValue(row.passCount);
            comparison.afterFailTotal += intValue(row.failCount);
        }
    }

    comparison.passDelta = comparison.afterPassTotal - comparison.beforePassTotal;
    comparison.failDelta = comparison.afterFailTotal - comparison.beforeFailTotal;

    for (const QString& module : order)
    {
        const bool hasBefore = beforeByModule.contains(module);
        const bool hasAfter = afterByModule.contains(module);
        const TestgridRow before = beforeByModule.value(module);
        const TestgridRow after = afterByModule.value(module);
        const int beforeFail = intValue(before.failCount);
        const int afterFail = intValue(after.failCount);
        const int failDelta = afterFail - beforeFail;
        QString status = QStringLiteral("unchanged");
        if (!hasBefore)
        {
            status = QStringLiteral("new_after");
        }
        else if (!hasAfter)
        {
            status = QStringLiteral("missing_after");
        }
        else if (failDelta > 0 && beforeFail == 0)
        {
            status = QStringLiteral("new_failure");
        }
        else if (failDelta > 0)
        {
            status = QStringLiteral("regressed");
        }
        else if (failDelta < 0)
        {
            status = QStringLiteral("improved");
        }
        else if (afterFail > 0)
        {
            status = QStringLiteral("unchanged_failed");
        }
        else
        {
            status = QStringLiteral("unchanged_passed");
        }

        comparison.rows.push_back({
            module,
            before.runCount,
            before.passCount,
            before.failCount,
            after.runCount,
            after.passCount,
            after.failCount,
            intValue(after.passCount) - intValue(before.passCount),
            failDelta,
            status,
        });
    }

    return comparison;
}

QVector<VerificationFailureDetail> VerificationResultParser::failureDetailsForTestgridRows(const QVector<TestgridRow>& rows,
                                                                                            const QString& artifact)
{
    QVector<VerificationFailureDetail> details;
    for (const TestgridRow& row : rows)
    {
        if (isAggregateModule(row.module) || intValue(row.failCount) <= 0)
        {
            continue;
        }

        details.push_back({
            QStringLiteral("testgrid"),
            row.module,
            QStringLiteral("failed"),
            QStringLiteral("%1 failed out of %2").arg(row.failCount, row.runCount),
            artifact,
        });
    }
    return details;
}

QVector<VerificationFailureDetail> VerificationResultParser::failureDetailsForTestdiff(const TestdiffSummary& summary,
                                                                                       const QString& artifact)
{
    QVector<VerificationFailureDetail> details;
    for (const TestdiffEntry& entry : summary.entries)
    {
        const QString status = entry.status.toLower();
        if (!status.contains(QStringLiteral("fail"))
            && !status.contains(QStringLiteral("error"))
            && !status.contains(QStringLiteral("diff"))
            && !status.contains(QStringLiteral("changed")))
        {
            continue;
        }

        const QString note = QStringList {entry.metric, entry.note}
            .join(QLatin1Char(' '))
            .trimmed();
        details.push_back({
            QStringLiteral("testdiff"),
            entry.name,
            entry.status,
            note.isEmpty() ? QStringLiteral("testdiff item requires review") : note,
            artifact,
        });
    }
    return details;
}

QVector<VerificationFailureDetail> VerificationResultParser::failureDetailsForComparison(const TestgridComparison& comparison,
                                                                                         const QString& artifact)
{
    QVector<VerificationFailureDetail> details;
    for (const TestgridComparisonRow& row : comparison.rows)
    {
        if (isAggregateModule(row.module) || row.failDelta <= 0)
        {
            continue;
        }

        details.push_back({
            QStringLiteral("before_after"),
            row.module,
            row.status.isEmpty() ? QStringLiteral("regressed") : row.status,
            QStringLiteral("fail %1 -> %2 (delta %3)")
                .arg(row.beforeFailCount, row.afterFailCount)
                .arg(row.failDelta),
            artifact,
        });
    }
    return details;
}

VerificationTimingSummary VerificationResultParser::timingSummary(const QVector<VerificationTimingEntry>& entries)
{
    VerificationTimingSummary summary;
    summary.entries = entries;
    for (const VerificationTimingEntry& entry : entries)
    {
        summary.totalElapsedMs += entry.elapsedMs;
    }
    return summary;
}
} // namespace occtdebug
