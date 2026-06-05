#pragma once

#include "core/case/CaseManifest.h"
#include "core/config/ConfigService.h"

#include <QString>
#include <QStringList>

#include <optional>

namespace occtdebug
{
struct CaseWorkspaceInfo
{
    QString caseId;
    QString rootPath;
    QString manifestPath;
    QStringList standardDirectories;
};

class CaseWorkspaceService
{
public:
    explicit CaseWorkspaceService(WorkbenchConfig config);

    CaseWorkspaceInfo workspaceInfo(const QString& caseId) const;
    bool ensureWorkspace(const QString& caseId, QString* error = nullptr) const;
    bool saveManifest(const CaseManifest& manifest, QString* error = nullptr) const;
    std::optional<CaseManifest> loadManifest(const QString& caseId, QString* error = nullptr) const;
    bool createFromSample(const QString& sampleDirectory, QString* error = nullptr) const;

private:
    static QString sanitizeCaseId(const QString& caseId);
    static bool copyDirectory(const QString& sourcePath, const QString& targetPath, QString* error);

    WorkbenchConfig m_config;
};
} // namespace occtdebug
