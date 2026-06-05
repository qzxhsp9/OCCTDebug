#include "core/config/ConfigService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>
#include <QProcessEnvironment>

namespace occtdebug
{
namespace
{
QString stringValue(const QJsonObject& object, const char* key, const QString& fallback = QString())
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isString() ? value.toString() : fallback;
}

int intValue(const QJsonObject& object, const char* key, int fallback = 0)
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isDouble() ? value.toInt() : fallback;
}

bool boolValue(const QJsonObject& object, const char* key, bool fallback = false)
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isBool() ? value.toBool() : fallback;
}

QJsonObject objectValue(const QJsonObject& object, const char* key)
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isObject() ? value.toObject() : QJsonObject {};
}
} // namespace

ConfigService::ConfigService(QString repoRoot)
    : m_repoRoot(QDir::cleanPath(std::move(repoRoot)))
{
    m_config.repoRoot = m_repoRoot;
    m_config.defaultConfigPath = QDir(m_repoRoot).absoluteFilePath(QStringLiteral("config/workbench.default.yaml"));
    m_config.localConfigPath = QDir(m_repoRoot).absoluteFilePath(QStringLiteral("config/workbench.local.yaml"));
}

bool ConfigService::load(QString* error)
{
    const auto defaultObject = readJsonObject(m_config.defaultConfigPath, error);
    if (!defaultObject.has_value())
    {
        return false;
    }

    QJsonObject merged = *defaultObject;
    m_config.loadedFiles.append(m_config.defaultConfigPath);

    if (QFileInfo::exists(m_config.localConfigPath))
    {
        const auto localObject = readJsonObject(m_config.localConfigPath, error);
        if (!localObject.has_value())
        {
            return false;
        }
        merged = mergeObjects(merged, *localObject);
        m_config.loadedFiles.append(m_config.localConfigPath);
    }
    else
    {
        m_config.warnings.append(QStringLiteral("Local config not found; using defaults: %1").arg(m_config.localConfigPath));
    }

    applyObject(merged);
    applyDerivedDefaults();
    return true;
}

const WorkbenchConfig& ConfigService::config() const
{
    return m_config;
}

std::optional<QJsonObject> ConfigService::readJsonObject(const QString& filePath, QString* error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot open config %1: %2").arg(filePath, file.errorString());
        }
        return std::nullopt;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("invalid JSON-compatible YAML config %1: %2").arg(filePath, parseError.errorString());
        }
        return std::nullopt;
    }

    return document.object();
}

QJsonObject ConfigService::mergeObjects(const QJsonObject& base, const QJsonObject& overrideObject)
{
    QJsonObject merged = base;
    for (auto it = overrideObject.begin(); it != overrideObject.end(); ++it)
    {
        if (it.value().isObject() && merged.value(it.key()).isObject())
        {
            merged.insert(it.key(), mergeObjects(merged.value(it.key()).toObject(), it.value().toObject()));
        }
        else
        {
            merged.insert(it.key(), it.value());
        }
    }
    return merged;
}

QString ConfigService::absolutePath(const QString& path) const
{
    if (path.isEmpty())
    {
        return {};
    }

    QFileInfo info(path);
    if (info.isAbsolute())
    {
        return QDir::cleanPath(info.absoluteFilePath());
    }
    return QDir::cleanPath(QDir(m_repoRoot).absoluteFilePath(path));
}

QString ConfigService::findDrawExecutable(const QString& occtRoot, const QString& buildType) const
{
    if (occtRoot.isEmpty())
    {
        return {};
    }

    const QStringList candidates {
        QStringLiteral("lib/%1/bind/DRAWEXE.exe").arg(buildType),
        QStringLiteral("lib/%1/bin/DRAWEXE.exe").arg(buildType),
        QStringLiteral("lib/%1/bini/DRAWEXE.exe").arg(buildType),
        QStringLiteral("lib/Debug/bind/DRAWEXE.exe"),
        QStringLiteral("lib/Release/bin/DRAWEXE.exe"),
        QStringLiteral("lib/RelWithDebInfo/bini/DRAWEXE.exe"),
    };

    const QDir root(occtRoot);
    for (const QString& candidate : candidates)
    {
        const QString path = QDir::cleanPath(root.absoluteFilePath(candidate));
        if (QFileInfo::exists(path))
        {
            return path;
        }
    }
    return {};
}

void ConfigService::applyObject(const QJsonObject& object)
{
    m_config.schemaVersion = intValue(object, "schema_version", 1);

    const QJsonObject workspace = objectValue(object, "workspace");
    m_config.buildRoot = absolutePath(stringValue(workspace, "build_root", QStringLiteral("out/build/debug")));
    m_config.caseRoot = absolutePath(stringValue(workspace, "case_root", QStringLiteral("cases")));
    m_config.artifactRoot = absolutePath(stringValue(workspace, "artifact_root", QStringLiteral("artifacts")));

    const QJsonObject occt = objectValue(object, "occt");
    m_config.occtBundledRoot = absolutePath(stringValue(occt, "bundled_root", QStringLiteral("depends/occt")));
    m_config.occtSourceRoot = absolutePath(stringValue(occt, "source_root"));
    m_config.occtBuildRoot = absolutePath(stringValue(occt, "build_root"));
    m_config.occtInstallRoot = absolutePath(stringValue(occt, "install_root", QStringLiteral("depends/occt")));
    m_config.casroot = absolutePath(stringValue(occt, "casroot", QStringLiteral("depends/occt")));
    m_config.drawExe = absolutePath(stringValue(occt, "drawexe"));

    const QJsonObject thirdParty = objectValue(object, "third_party");
    m_config.freetypeRoot = absolutePath(stringValue(thirdParty, "freetype_root", QStringLiteral("depends/occt_3rdparty/freetype-2.13.3-x64")));
    m_config.tcltkRoot = absolutePath(stringValue(thirdParty, "tcltk_root", QStringLiteral("depends/occt_3rdparty/tcltk-8.6.15-x64")));

    const QJsonObject qt = objectValue(object, "qt");
    m_config.qtRoot = absolutePath(stringValue(qt, "root"));

    const QJsonObject compiler = objectValue(object, "compiler");
    m_config.developerCommand = absolutePath(stringValue(compiler, "developer_command"));
    m_config.generator = stringValue(compiler, "generator", QStringLiteral("Ninja"));
    m_config.architecture = stringValue(compiler, "architecture", QStringLiteral("x64"));

    const QJsonObject runtime = objectValue(object, "runtime");
    m_config.defaultBuildType = stringValue(runtime, "default_build_type", QStringLiteral("Debug"));
    m_config.maxParallelJobs = intValue(runtime, "max_parallel_jobs", 0);

    const QJsonObject verify = objectValue(object, "verify");
    const QJsonObject testgrid = objectValue(verify, "testgrid");
    m_config.testgridRoot = absolutePath(stringValue(testgrid, "root"));
    m_config.testgridExecutable = absolutePath(stringValue(testgrid, "executable"));
    m_config.testgridArguments = stringValue(testgrid, "arguments");
    m_config.testgridGroup = stringValue(testgrid, "group");
    m_config.testgridGrid = stringValue(testgrid, "grid");
    m_config.testgridCase = stringValue(testgrid, "case");
    m_config.testdiffExecutable = absolutePath(stringValue(testgrid, "testdiff_executable"));
    m_config.testdiffArguments = stringValue(testgrid, "testdiff_arguments");
    m_config.testdiffOutputRoot = absolutePath(stringValue(testgrid, "testdiff_output_root"));

    const QJsonObject patch = objectValue(object, "patch");
    m_config.patchWorktreeRoot = absolutePath(stringValue(patch, "worktree_root"));

    const QJsonObject privacy = objectValue(object, "privacy");
    m_config.defaultCaseLevel = stringValue(privacy, "default_case_level", QStringLiteral("internal"));
    m_config.allowExternalNetwork = boolValue(privacy, "allow_external_network", false);
}

void ConfigService::applyDerivedDefaults()
{
    if (m_config.qtRoot.isEmpty())
    {
        const QString qtdir = QProcessEnvironment::systemEnvironment().value(QStringLiteral("QTDIR"));
        if (!qtdir.isEmpty())
        {
            m_config.qtRoot = QDir::cleanPath(qtdir);
        }
    }

    if (m_config.drawExe.isEmpty())
    {
        m_config.drawExe = findDrawExecutable(m_config.occtInstallRoot, m_config.defaultBuildType);
    }

    if (m_config.drawExe.isEmpty())
    {
        m_config.warnings.append(QStringLiteral("DRAWEXE.exe was not found under %1").arg(m_config.occtInstallRoot));
    }
}
} // namespace occtdebug
