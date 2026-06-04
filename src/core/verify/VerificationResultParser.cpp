#include "core/verify/VerificationResultParser.h"

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
} // namespace occtdebug
