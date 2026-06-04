#include "core/source/SourceIndex.h"

#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace occtdebug
{
QStringList SourceIndex::defaultNameFilters()
{
    return {
        QStringLiteral("*.c"),
        QStringLiteral("*.cc"),
        QStringLiteral("*.cpp"),
        QStringLiteral("*.cxx"),
        QStringLiteral("*.h"),
        QStringLiteral("*.hpp"),
        QStringLiteral("*.hxx"),
        QStringLiteral("*.tcl"),
        QStringLiteral("*.md"),
    };
}

SourceIndex SourceIndex::build(const QString& rootDirectory, const QStringList& nameFilters, qsizetype maxFileBytes)
{
    SourceIndex index;
    QDirIterator it(rootDirectory, nameFilters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        const QString filePath = it.next();
        const QFileInfo info(filePath);
        if (info.size() > maxFileBytes)
        {
            continue;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            continue;
        }

        QTextStream stream(&file);
        int lineNumber = 0;
        while (!stream.atEnd())
        {
            ++lineNumber;
            const QString line = stream.readLine();
            if (!line.trimmed().isEmpty())
            {
                index.m_entries.push_back({filePath, lineNumber, line});
            }
        }
    }

    return index;
}

QVector<SourceIndexEntry> SourceIndex::search(const QString& query, int limit) const
{
    QVector<SourceIndexEntry> results;
    const QString needle = query.trimmed();
    if (needle.isEmpty() || limit <= 0)
    {
        return results;
    }

    for (const SourceIndexEntry& entry : m_entries)
    {
        if (entry.text.contains(needle, Qt::CaseInsensitive)
            || entry.filePath.contains(needle, Qt::CaseInsensitive))
        {
            results.push_back(entry);
            if (results.size() >= limit)
            {
                break;
            }
        }
    }

    return results;
}

const QVector<SourceIndexEntry>& SourceIndex::entries() const
{
    return m_entries;
}
} // namespace occtdebug
