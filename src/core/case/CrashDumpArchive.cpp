#include "core/case/CrashDumpArchive.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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
    return path;
}

QString safeFileName(QString name)
{
    name = QFileInfo(name).fileName().trimmed();
    if (name.isEmpty())
    {
        name = QStringLiteral("crash.dmp");
    }
    name.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]+")), QStringLiteral("_"));
    return name;
}

QString sha256Hex(const QString& path, QString* error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot read crash dump for hashing: %1").arg(file.errorString());
        }
        return QString();
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot hash crash dump");
        }
        return QString();
    }
    return QString::fromLatin1(hash.result().toHex());
}

bool writeJson(const QString& path, const QJsonObject& object, QString* error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot write crash dump manifest: %1").arg(file.errorString());
        }
        return false;
    }
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    if (!file.commit())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot commit crash dump manifest: %1").arg(file.errorString());
        }
        return false;
    }
    return true;
}
} // namespace

bool CrashDumpArchive::archiveFile(const QString& sourcePath,
                                   const QString& workspaceRoot,
                                   const QString& caseId,
                                   CrashDumpArchiveResult* result,
                                   QString* error)
{
    if (result == nullptr)
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("crash dump archive result pointer is null");
        }
        return false;
    }
    *result = {};

    const QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.exists() || !sourceInfo.isFile())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("crash dump file does not exist: %1").arg(sourcePath);
        }
        return false;
    }
    if (workspaceRoot.trimmed().isEmpty())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("case workspace root is empty");
        }
        return false;
    }

    QDir workspace(workspaceRoot);
    const QString crashDirRelative = QStringLiteral("artifacts/crash");
    if (!workspace.mkpath(crashDirRelative))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot create crash artifact directory");
        }
        return false;
    }

    const QString timestamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMddTHHmmsszzzZ"));
    const QString targetName = QStringLiteral("%1_%2").arg(timestamp, safeFileName(sourceInfo.fileName()));
    const QString targetRelative = normalizedPath(QStringLiteral("%1/%2").arg(crashDirRelative, targetName));
    const QString targetPath = workspace.filePath(targetRelative);

    if (QFileInfo(targetPath).exists() && !QFile::remove(targetPath))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot replace existing crash dump artifact");
        }
        return false;
    }
    if (!QFile::copy(sourceInfo.absoluteFilePath(), targetPath))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot copy crash dump into case workspace");
        }
        return false;
    }

    QString hashError;
    const QString hash = sha256Hex(targetPath, &hashError);
    if (hash.isEmpty())
    {
        if (error != nullptr)
        {
            *error = hashError;
        }
        return false;
    }

    const QFileInfo targetInfo(targetPath);
    const QString manifestRelative = normalizedPath(QStringLiteral("%1/%2.json").arg(crashDirRelative, targetName));
    const QString manifestPath = workspace.filePath(manifestRelative);
    const QString archivedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    const QJsonObject manifest {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("case_id"), caseId},
        {QStringLiteral("type"), QStringLiteral("crash_dump")},
        {QStringLiteral("artifact"), targetRelative},
        {QStringLiteral("original_name"), sourceInfo.fileName()},
        {QStringLiteral("bytes"), static_cast<double>(targetInfo.size())},
        {QStringLiteral("sha256"), hash},
        {QStringLiteral("archived_at"), archivedAt},
    };
    if (!writeJson(manifestPath, manifest, error))
    {
        return false;
    }

    result->artifactRelativePath = targetRelative;
    result->manifestRelativePath = manifestRelative;
    result->originalName = sourceInfo.fileName();
    result->sha256 = hash;
    result->bytes = targetInfo.size();
    result->archivedAt = archivedAt;
    result->manifest = manifest;
    return true;
}
} // namespace occtdebug
