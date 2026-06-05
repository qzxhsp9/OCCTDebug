#include "core/verify/TestdiffArtifactScanner.h"

#include "core/verify/TestdiffArtifactAnalysis.h"
#include "core/verify/TestdiffArtifactIndex.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonArray>
#include <QSet>
#include <QStringList>
#include <QVector>

#include <algorithm>

namespace occtdebug
{
namespace
{
struct ScanDirectory
{
    QString role;
    QString path;
};

QString normalizedPath(QString value)
{
    return value.replace(QLatin1Char('\\'), QLatin1Char('/'));
}

QString relativeExistingPath(const QString& workspaceRoot, const QString& path)
{
    if (path.isEmpty() || !QFileInfo::exists(path))
    {
        return QString();
    }
    return normalizedPath(QDir(workspaceRoot).relativeFilePath(path));
}

QString roleFromDirectoryName(const QString& value)
{
    const QString lower = value.toLower();
    if (lower.contains(QStringLiteral("before")))
    {
        return QStringLiteral("before");
    }
    if (lower.contains(QStringLiteral("after")))
    {
        return QStringLiteral("after");
    }
    if (lower.contains(QStringLiteral("diff")) || lower.contains(QStringLiteral("delta")))
    {
        return QStringLiteral("diff");
    }
    return QStringLiteral("other");
}

QString artifactKind(const QFileInfo& info)
{
    const QString suffix = info.suffix().toLower();
    const QString name = info.fileName().toLower();
    const QSet<QString> images {
        QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"),
        QStringLiteral("bmp"), QStringLiteral("gif"), QStringLiteral("webp"),
        QStringLiteral("svg"),
    };
    const QSet<QString> properties {
        QStringLiteral("json"), QStringLiteral("xml"), QStringLiteral("yaml"),
        QStringLiteral("yml"), QStringLiteral("csv"), QStringLiteral("properties"),
        QStringLiteral("brep"), QStringLiteral("brp"),
    };
    const QSet<QString> logs {
        QStringLiteral("log"), QStringLiteral("out"), QStringLiteral("err"),
    };
    const QSet<QString> text {
        QStringLiteral("txt"), QStringLiteral("md"), QStringLiteral("html"),
        QStringLiteral("htm"),
    };

    if (images.contains(suffix))
    {
        return QStringLiteral("image");
    }
    if (logs.contains(suffix))
    {
        return QStringLiteral("log");
    }
    if (name.contains(QStringLiteral("perf")) || name.contains(QStringLiteral("timing")) || name.contains(QStringLiteral("benchmark")))
    {
        return QStringLiteral("performance");
    }
    if (properties.contains(suffix))
    {
        return QStringLiteral("property");
    }
    if (text.contains(suffix))
    {
        return QStringLiteral("text");
    }
    return QStringLiteral("other");
}

void appendCandidate(QVector<ScanDirectory>& directories,
                     QSet<QString>& seen,
                     const QString& workspaceRoot,
                     const QString& role,
                     const QString& relativePath)
{
    const QString absolute = QDir(workspaceRoot).filePath(relativePath);
    const QFileInfo info(absolute);
    if (!info.exists() || !info.isDir())
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
    directories.push_back({role, info.absoluteFilePath()});
}

QVector<ScanDirectory> candidateDirectories(const QString& summaryPath, const QString& workspaceRoot)
{
    QVector<ScanDirectory> directories;
    QSet<QString> seen;

    appendCandidate(directories, seen, workspaceRoot, QStringLiteral("before"), QStringLiteral("verification/testdiff/before"));
    appendCandidate(directories, seen, workspaceRoot, QStringLiteral("after"), QStringLiteral("verification/testdiff/after"));
    appendCandidate(directories, seen, workspaceRoot, QStringLiteral("diff"), QStringLiteral("verification/testdiff/diff"));
    appendCandidate(directories, seen, workspaceRoot, QStringLiteral("before"), QStringLiteral("verification/testdiff_before"));
    appendCandidate(directories, seen, workspaceRoot, QStringLiteral("after"), QStringLiteral("verification/testdiff_after"));
    appendCandidate(directories, seen, workspaceRoot, QStringLiteral("diff"), QStringLiteral("verification/testdiff_diff"));
    appendCandidate(directories, seen, workspaceRoot, QStringLiteral("before"), QStringLiteral("artifacts/testdiff/before"));
    appendCandidate(directories, seen, workspaceRoot, QStringLiteral("after"), QStringLiteral("artifacts/testdiff/after"));
    appendCandidate(directories, seen, workspaceRoot, QStringLiteral("diff"), QStringLiteral("artifacts/testdiff/diff"));
    appendCandidate(directories, seen, workspaceRoot, QStringLiteral("before"), QStringLiteral("artifacts/testdiff_before"));
    appendCandidate(directories, seen, workspaceRoot, QStringLiteral("after"), QStringLiteral("artifacts/testdiff_after"));
    appendCandidate(directories, seen, workspaceRoot, QStringLiteral("diff"), QStringLiteral("artifacts/testdiff_diff"));

    const QFileInfo summaryInfo(summaryPath);
    if (summaryInfo.exists())
    {
        const QDir summaryDir(summaryInfo.absolutePath());
        const QFileInfoList children = summaryDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo& child : children)
        {
            const QString lower = child.fileName().toLower();
            if (!lower.contains(QStringLiteral("testdiff"))
                && !lower.contains(QStringLiteral("before"))
                && !lower.contains(QStringLiteral("after"))
                && !lower.contains(QStringLiteral("diff")))
            {
                continue;
            }
            const QString canonical = child.canonicalFilePath();
            const QString key = canonical.isEmpty() ? child.absoluteFilePath() : canonical;
            if (seen.contains(key))
            {
                continue;
            }
            seen.insert(key);
            directories.push_back({roleFromDirectoryName(child.fileName()), child.absoluteFilePath()});
        }
    }
    return directories;
}

void increment(QJsonObject& object, const QString& key)
{
    object.insert(key, object.value(key).toInt() + 1);
}
} // namespace

QJsonObject TestdiffArtifactScanner::buildManifest(const QString& summaryPath,
                                                   const QString& workspaceRoot,
                                                   const QString& commandStdout,
                                                   const QString& commandStderr,
                                                   int entriesCount,
                                                   int changedCount,
                                                   int failedCount)
{
    QJsonObject out {
        {QStringLiteral("entries_count"), entriesCount},
        {QStringLiteral("changed_count"), changedCount},
        {QStringLiteral("failed_count"), failedCount},
    };

    const QString summary = relativeExistingPath(workspaceRoot, summaryPath);
    if (!summary.isEmpty())
    {
        out.insert(QStringLiteral("summary"), summary);
    }
    if (!commandStdout.isEmpty())
    {
        out.insert(QStringLiteral("command_stdout"), normalizedPath(commandStdout));
    }
    if (!commandStderr.isEmpty())
    {
        out.insert(QStringLiteral("command_stderr"), normalizedPath(commandStderr));
    }

    QJsonArray directoriesJson;
    QJsonArray filesJson;
    QJsonObject counts {
        {QStringLiteral("total"), 0},
        {QStringLiteral("before"), 0},
        {QStringLiteral("after"), 0},
        {QStringLiteral("diff"), 0},
        {QStringLiteral("other"), 0},
        {QStringLiteral("image"), 0},
        {QStringLiteral("property"), 0},
        {QStringLiteral("performance"), 0},
        {QStringLiteral("log"), 0},
        {QStringLiteral("text"), 0},
    };

    constexpr int maxFiles = 512;
    bool truncated = false;
    const QVector<ScanDirectory> directories = candidateDirectories(summaryPath, workspaceRoot);
    for (const ScanDirectory& directory : directories)
    {
        const QString relativeDirectory = relativeExistingPath(workspaceRoot, directory.path);
        if (relativeDirectory.isEmpty())
        {
            continue;
        }
        directoriesJson.append(QJsonObject {
            {QStringLiteral("role"), directory.role},
            {QStringLiteral("path"), relativeDirectory},
        });

        QStringList files;
        QDirIterator it(directory.path, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext())
        {
            files.push_back(it.next());
        }
        std::sort(files.begin(), files.end(), [](const QString& left, const QString& right) {
            return normalizedPath(left) < normalizedPath(right);
        });

        for (const QString& filePath : files)
        {
            if (filesJson.size() >= maxFiles)
            {
                truncated = true;
                break;
            }
            const QFileInfo info(filePath);
            const QString relativeFile = relativeExistingPath(workspaceRoot, filePath);
            if (relativeFile.isEmpty())
            {
                continue;
            }
            const QString kind = artifactKind(info);
            filesJson.append(QJsonObject {
                {QStringLiteral("role"), directory.role},
                {QStringLiteral("kind"), kind},
                {QStringLiteral("path"), relativeFile},
                {QStringLiteral("bytes"), static_cast<double>(info.size())},
            });
            increment(counts, QStringLiteral("total"));
            increment(counts, directory.role);
            increment(counts, kind);
        }
        if (truncated)
        {
            break;
        }
    }

    out.insert(QStringLiteral("directories"), directoriesJson);
    out.insert(QStringLiteral("artifact_files"), filesJson);
    out.insert(QStringLiteral("artifact_counts"), counts);
    const QJsonObject artifactIndex = TestdiffArtifactIndex::build(filesJson);
    out.insert(QStringLiteral("artifact_index"), artifactIndex);
    out.insert(QStringLiteral("artifact_analysis"), TestdiffArtifactAnalysis::build(workspaceRoot, artifactIndex));
    out.insert(QStringLiteral("truncated"), truncated);
    return out;
}
} // namespace occtdebug
