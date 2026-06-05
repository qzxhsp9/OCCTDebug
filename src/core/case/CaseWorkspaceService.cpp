#include "core/case/CaseWorkspaceService.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

namespace occtdebug
{
namespace
{
QStringList standardCaseDirectories()
{
    return {
        QStringLiteral("input"),
        QStringLiteral("repro"),
        QStringLiteral("env"),
        QStringLiteral("logs"),
        QStringLiteral("artifacts"),
        QStringLiteral("report"),
        QStringLiteral("verification"),
    };
}
} // namespace

CaseWorkspaceService::CaseWorkspaceService(WorkbenchConfig config)
    : m_config(config)
{
}

CaseWorkspaceInfo CaseWorkspaceService::workspaceInfo(const QString& caseId) const
{
    const QString safeCaseId = sanitizeCaseId(caseId);
    const QString rootPath = QDir::cleanPath(QDir(m_config.caseRoot).absoluteFilePath(safeCaseId));

    CaseWorkspaceInfo info;
    info.caseId = safeCaseId;
    info.rootPath = rootPath;
    info.manifestPath = QDir(rootPath).absoluteFilePath(QStringLiteral("case.json"));
    info.standardDirectories = standardCaseDirectories();
    return info;
}

bool CaseWorkspaceService::ensureWorkspace(const QString& caseId, QString* error) const
{
    const CaseWorkspaceInfo info = workspaceInfo(caseId);
    QDir root;
    if (!root.mkpath(info.rootPath))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot create case workspace: %1").arg(info.rootPath);
        }
        return false;
    }

    const QDir workspace(info.rootPath);
    for (const QString& relativeDirectory : info.standardDirectories)
    {
        if (!workspace.mkpath(relativeDirectory))
        {
            if (error != nullptr)
            {
                *error = QStringLiteral("cannot create case subdirectory %1 in %2").arg(relativeDirectory, info.rootPath);
            }
            return false;
        }
    }
    return true;
}

bool CaseWorkspaceService::saveManifest(const CaseManifest& manifest, QString* error) const
{
    if (!ensureWorkspace(manifest.caseId, error))
    {
        return false;
    }

    const CaseWorkspaceInfo info = workspaceInfo(manifest.caseId);
    return manifest.saveToFile(info.manifestPath, error);
}

std::optional<CaseManifest> CaseWorkspaceService::loadManifest(const QString& caseId, QString* error) const
{
    const CaseWorkspaceInfo info = workspaceInfo(caseId);
    if (!QFileInfo::exists(info.manifestPath))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("case manifest does not exist: %1").arg(info.manifestPath);
        }
        return std::nullopt;
    }
    return CaseManifest::loadFromFile(info.manifestPath, error);
}

bool CaseWorkspaceService::createFromSample(const QString& sampleDirectory, QString* error) const
{
    const QString sampleManifestPath = QDir(sampleDirectory).absoluteFilePath(QStringLiteral("case.json"));
    const std::optional<CaseManifest> manifest = CaseManifest::loadFromFile(sampleManifestPath, error);
    if (!manifest.has_value())
    {
        return false;
    }

    const CaseWorkspaceInfo info = workspaceInfo(manifest->caseId);
    if (QFileInfo::exists(info.manifestPath))
    {
        return true;
    }

    if (!ensureWorkspace(manifest->caseId, error))
    {
        return false;
    }

    if (!copyDirectory(sampleDirectory, info.rootPath, error))
    {
        return false;
    }

    return manifest->saveToFile(info.manifestPath, error);
}

QString CaseWorkspaceService::sanitizeCaseId(const QString& caseId)
{
    QString out = caseId.trimmed();
    out.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_.-]")), QStringLiteral("_"));
    return out.isEmpty() ? QStringLiteral("OCC-LOCAL-UNNAMED") : out;
}

bool CaseWorkspaceService::copyDirectory(const QString& sourcePath, const QString& targetPath, QString* error)
{
    const QDir source(sourcePath);
    if (!source.exists())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("sample directory does not exist: %1").arg(sourcePath);
        }
        return false;
    }

    QDir target;
    if (!target.mkpath(targetPath))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot create target directory: %1").arg(targetPath);
        }
        return false;
    }

    QDirIterator it(sourcePath, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        const QString sourceFilePath = it.next();
        const QString relativePath = source.relativeFilePath(sourceFilePath);
        const QString targetFilePath = QDir(targetPath).absoluteFilePath(relativePath);
        const QFileInfo targetInfo(targetFilePath);
        if (!QDir().mkpath(targetInfo.absolutePath()))
        {
            if (error != nullptr)
            {
                *error = QStringLiteral("cannot create directory for %1").arg(targetFilePath);
            }
            return false;
        }
        if (QFileInfo::exists(targetFilePath) && !QFile::remove(targetFilePath))
        {
            if (error != nullptr)
            {
                *error = QStringLiteral("cannot replace existing file: %1").arg(targetFilePath);
            }
            return false;
        }
        if (!QFile::copy(sourceFilePath, targetFilePath))
        {
            if (error != nullptr)
            {
                *error = QStringLiteral("cannot copy %1 to %2").arg(sourceFilePath, targetFilePath);
            }
            return false;
        }
    }
    return true;
}
} // namespace occtdebug
