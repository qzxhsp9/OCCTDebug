#include "core/verify/TestdiffRunnerAdapter.h"

#include "core/verify/TestdiffArtifactScanner.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QSet>
#include <QVector>

namespace occtdebug
{
namespace
{
struct RoleDirectory
{
    QString role;
    QString path;
};

QString normalizedPath(QString path)
{
    return path.replace(QLatin1Char('\\'), QLatin1Char('/'));
}

QString roleForName(const QString& name)
{
    const QString lower = name.toLower();
    if (lower == QStringLiteral("before") || lower.contains(QStringLiteral("baseline")))
    {
        return QStringLiteral("before");
    }
    if (lower == QStringLiteral("after") || lower.contains(QStringLiteral("patched")))
    {
        return QStringLiteral("after");
    }
    if (lower == QStringLiteral("diff") || lower == QStringLiteral("delta") || lower.contains(QStringLiteral("compare")))
    {
        return QStringLiteral("diff");
    }
    return QString();
}

void appendRoleDirectory(QVector<RoleDirectory>& out, QSet<QString>& seen, const QString& role, const QString& path)
{
    const QFileInfo info(path);
    if (role.isEmpty() || !info.exists() || !info.isDir())
    {
        return;
    }
    const QString canonical = info.canonicalFilePath();
    const QString key = canonical.isEmpty() ? info.absoluteFilePath() : canonical;
    if (seen.contains(key))
    {
        return;
    }
    seen.insert(key);
    out.push_back({role, info.absoluteFilePath()});
}

QVector<RoleDirectory> findRoleDirectories(const QString& runnerOutputRoot)
{
    QVector<RoleDirectory> out;
    QSet<QString> seen;
    const QDir root(runnerOutputRoot);
    const QStringList directCandidates {
        QStringLiteral("before"),
        QStringLiteral("after"),
        QStringLiteral("diff"),
        QStringLiteral("delta"),
        QStringLiteral("testdiff/before"),
        QStringLiteral("testdiff/after"),
        QStringLiteral("testdiff/diff"),
    };
    for (const QString& candidate : directCandidates)
    {
        appendRoleDirectory(out, seen, roleForName(QFileInfo(candidate).fileName()), root.filePath(candidate));
    }

    const QFileInfoList children = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo& child : children)
    {
        appendRoleDirectory(out, seen, roleForName(child.fileName()), child.absoluteFilePath());
    }
    return out;
}

bool copyRoleDirectory(const QString& sourceDirectory,
                       const QString& targetDirectory,
                       int* copiedFiles,
                       QString* error)
{
    QDir targetRoot;
    if (!targetRoot.mkpath(targetDirectory))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("failed to create testdiff artifact directory: %1").arg(targetDirectory);
        }
        return false;
    }

    const QDir sourceRoot(sourceDirectory);
    QDirIterator it(sourceDirectory, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        const QString sourcePath = it.next();
        const QString relativePath = sourceRoot.relativeFilePath(sourcePath);
        const QString targetPath = QDir(targetDirectory).filePath(relativePath);
        if (!targetRoot.mkpath(QFileInfo(targetPath).absolutePath()))
        {
            if (error != nullptr)
            {
                *error = QStringLiteral("failed to create testdiff artifact directory: %1").arg(QFileInfo(targetPath).absolutePath());
            }
            return false;
        }
        if (QFileInfo::exists(targetPath) && !QFile::remove(targetPath))
        {
            if (error != nullptr)
            {
                *error = QStringLiteral("failed to replace testdiff artifact: %1").arg(targetPath);
            }
            return false;
        }
        if (!QFile::copy(sourcePath, targetPath))
        {
            if (error != nullptr)
            {
                *error = QStringLiteral("failed to copy testdiff artifact %1 to %2").arg(sourcePath, targetPath);
            }
            return false;
        }
        if (copiedFiles != nullptr)
        {
            ++(*copiedFiles);
        }
    }
    return true;
}
} // namespace

TestdiffRunnerImportResult TestdiffRunnerAdapter::importOutput(const QString& workspaceRoot,
                                                               const QString& runnerOutputRoot,
                                                               const QString& summaryPath,
                                                               const QString& commandStdout,
                                                               const QString& commandStderr,
                                                               int entriesCount,
                                                               int changedCount,
                                                               int failedCount)
{
    TestdiffRunnerImportResult result;
    const QFileInfo sourceInfo(runnerOutputRoot);
    if (!sourceInfo.exists() || !sourceInfo.isDir())
    {
        result.error = QStringLiteral("testdiff runner output directory does not exist");
        return result;
    }

    const QVector<RoleDirectory> directories = findRoleDirectories(sourceInfo.absoluteFilePath());
    if (directories.isEmpty())
    {
        result.error = QStringLiteral("testdiff runner output does not contain before/after/diff directories");
        return result;
    }

    const QDir workspace(workspaceRoot);
    for (const RoleDirectory& directory : directories)
    {
        const QString targetDirectory = workspace.filePath(QStringLiteral("artifacts/testdiff/%1").arg(directory.role));
        if (!copyRoleDirectory(directory.path, targetDirectory, &result.copiedFiles, &result.error))
        {
            return result;
        }
        result.importedDirectories.push_back(QStringLiteral("artifacts/testdiff/%1").arg(directory.role));
    }

    result.manifest = TestdiffArtifactScanner::buildManifest(summaryPath,
                                                            workspaceRoot,
                                                            commandStdout,
                                                            commandStderr,
                                                            entriesCount,
                                                            changedCount,
                                                            failedCount);
    QJsonObject adapter {
        {QStringLiteral("source_layout"), QStringLiteral("runner_output_before_after_diff")},
        {QStringLiteral("copied_files"), result.copiedFiles},
    };
    QJsonArray imported;
    for (const QString& path : result.importedDirectories)
    {
        imported.append(normalizedPath(path));
    }
    adapter.insert(QStringLiteral("imported_directories"), imported);
    result.manifest.insert(QStringLiteral("adapter"), adapter);
    result.success = true;
    return result;
}
} // namespace occtdebug
