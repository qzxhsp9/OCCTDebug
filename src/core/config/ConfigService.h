#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

#include <optional>

namespace occtdebug
{
struct WorkbenchConfig
{
    int schemaVersion = 1;

    QString repoRoot;
    QString buildRoot;
    QString caseRoot;
    QString artifactRoot;

    QString occtBundledRoot;
    QString occtSourceRoot;
    QString occtBuildRoot;
    QString occtInstallRoot;
    QString casroot;
    QString drawExe;

    QString freetypeRoot;
    QString tcltkRoot;
    QString qtRoot;

    QString developerCommand;
    QString generator;
    QString architecture;
    QString defaultBuildType;
    int maxParallelJobs = 0;

    QString testgridRoot;
    QString testgridExecutable;
    QString testgridArguments;
    QString testgridGroup;
    QString testgridGrid;
    QString testgridCase;
    QString testdiffExecutable;
    QString testdiffArguments;
    QString testdiffOutputRoot;

    QString patchWorktreeRoot;

    QString defaultCaseLevel;
    bool allowExternalNetwork = false;

    QString defaultConfigPath;
    QString localConfigPath;
    QStringList loadedFiles;
    QStringList warnings;
};

class ConfigService
{
public:
    explicit ConfigService(QString repoRoot);

    bool load(QString* error = nullptr);
    const WorkbenchConfig& config() const;

private:
    static std::optional<QJsonObject> readJsonObject(const QString& filePath, QString* error);
    static QJsonObject mergeObjects(const QJsonObject& base, const QJsonObject& overrideObject);

    QString absolutePath(const QString& path) const;
    QString findDrawExecutable(const QString& occtRoot, const QString& buildType) const;
    void applyObject(const QJsonObject& object);
    void applyDerivedDefaults();

    QString m_repoRoot;
    WorkbenchConfig m_config;
};
} // namespace occtdebug
