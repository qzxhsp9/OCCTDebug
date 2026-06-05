#pragma once

#include "core/case/CaseManifest.h"
#include "core/runner/CommandRunner.h"

#include <QJsonObject>
#include <QString>

namespace occtdebug
{
class TestgridArtifactService
{
public:
    static QString logRelativePath(const QString& fileName);
    static QString artifactRelativePath(const QString& fileName);
    static QString verificationRelativePath(const QString& fileName);

    static QString logPath(const QString& workspaceRoot, const QString& fileName);
    static QString artifactPath(const QString& workspaceRoot, const QString& fileName);
    static QString verificationPath(const QString& workspaceRoot, const QString& fileName);

    static bool ensureWorkspaceDirectories(const QString& workspaceRoot, QString* error = nullptr);
    static bool writeTextArtifact(const QString& absolutePath, const QString& text, QString* error = nullptr);
    static QString readTextArtifact(const QString& absolutePath);
    static bool writeJsonArtifact(const QString& absolutePath, const QJsonObject& json, QString* error = nullptr);
    static bool writeCommandLogs(const QString& workspaceRoot,
                                 const QString& stdoutFileName,
                                 const QString& stderrFileName,
                                 const CommandResult& result,
                                 QString* error = nullptr);
    static bool writePhaseSummary(const QString& workspaceRoot,
                                  const QString& phase,
                                  const QVector<TestgridRow>& rows,
                                  QString* absolutePath = nullptr,
                                  QString* error = nullptr);
    static QVector<TestgridRow> readPhaseRows(const QString& workspaceRoot, const QString& phase);
};
} // namespace occtdebug
