#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

namespace occtdebug
{
struct SourceIndexEntry
{
    QString filePath;
    int lineNumber = 0;
    QString text;
};

class SourceIndex
{
public:
    static QStringList defaultNameFilters();

    static SourceIndex build(const QString& rootDirectory,
                             const QStringList& nameFilters = defaultNameFilters(),
                             qsizetype maxFileBytes = 1024 * 1024);

    QVector<SourceIndexEntry> search(const QString& query, int limit = 50) const;
    const QVector<SourceIndexEntry>& entries() const;

private:
    QVector<SourceIndexEntry> m_entries;
};
} // namespace occtdebug
