#pragma once

#include <QJsonObject>
#include <QString>

namespace occtdebug
{
struct CrashDumpArchiveResult
{
    QString artifactRelativePath;
    QString manifestRelativePath;
    QString originalName;
    QString sha256;
    qint64 bytes = 0;
    QString archivedAt;
    QJsonObject manifest;
};

class CrashDumpArchive
{
public:
    static bool archiveFile(const QString& sourcePath,
                            const QString& workspaceRoot,
                            const QString& caseId,
                            CrashDumpArchiveResult* result,
                            QString* error = nullptr);
};
} // namespace occtdebug
