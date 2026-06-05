#include "core/verify/TestgridArtifactService.h"

#include "core/verify/VerificationResultParser.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>

namespace occtdebug
{
namespace
{
QString cleanFileName(QString fileName)
{
    fileName.replace(QLatin1Char('\\'), QLatin1Char('/'));
    while (fileName.startsWith(QStringLiteral("./")))
    {
        fileName.remove(0, 2);
    }
    return fileName;
}

QString joinRelative(const QString& directory, const QString& fileName)
{
    return QStringLiteral("%1/%2").arg(directory, cleanFileName(fileName));
}

QString joinAbsolute(const QString& workspaceRoot, const QString& directory, const QString& fileName)
{
    return QDir(workspaceRoot).filePath(joinRelative(directory, fileName));
}
} // namespace

QString TestgridArtifactService::logRelativePath(const QString& fileName)
{
    return joinRelative(QStringLiteral("logs"), fileName);
}

QString TestgridArtifactService::artifactRelativePath(const QString& fileName)
{
    return joinRelative(QStringLiteral("artifacts"), fileName);
}

QString TestgridArtifactService::verificationRelativePath(const QString& fileName)
{
    return joinRelative(QStringLiteral("verification"), fileName);
}

QString TestgridArtifactService::logPath(const QString& workspaceRoot, const QString& fileName)
{
    return joinAbsolute(workspaceRoot, QStringLiteral("logs"), fileName);
}

QString TestgridArtifactService::artifactPath(const QString& workspaceRoot, const QString& fileName)
{
    return joinAbsolute(workspaceRoot, QStringLiteral("artifacts"), fileName);
}

QString TestgridArtifactService::verificationPath(const QString& workspaceRoot, const QString& fileName)
{
    return joinAbsolute(workspaceRoot, QStringLiteral("verification"), fileName);
}

bool TestgridArtifactService::ensureWorkspaceDirectories(const QString& workspaceRoot, QString* error)
{
    QDir root(workspaceRoot);
    const QStringList directories {
        QStringLiteral("logs"),
        QStringLiteral("artifacts"),
        QStringLiteral("verification"),
    };
    for (const QString& directory : directories)
    {
        if (!root.mkpath(directory))
        {
            if (error != nullptr)
            {
                *error = QStringLiteral("failed to create %1").arg(root.filePath(directory));
            }
            return false;
        }
    }
    return true;
}

bool TestgridArtifactService::writeTextArtifact(const QString& absolutePath, const QString& text, QString* error)
{
    QDir dir;
    if (!dir.mkpath(QFileInfo(absolutePath).absolutePath()))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("failed to create artifact directory: %1").arg(QFileInfo(absolutePath).absolutePath());
        }
        return false;
    }

    QFile file(absolutePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("failed to write %1: %2").arg(absolutePath, file.errorString());
        }
        return false;
    }
    file.write(text.toUtf8());
    return true;
}

QString TestgridArtifactService::readTextArtifact(const QString& absolutePath)
{
    QFile file(absolutePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return QString();
    }
    return QString::fromUtf8(file.readAll());
}

bool TestgridArtifactService::writeJsonArtifact(const QString& absolutePath, const QJsonObject& json, QString* error)
{
    return writeTextArtifact(absolutePath, QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Indented)), error);
}

bool TestgridArtifactService::writeCommandLogs(const QString& workspaceRoot,
                                               const QString& stdoutFileName,
                                               const QString& stderrFileName,
                                               const CommandResult& result,
                                               QString* error)
{
    return writeTextArtifact(logPath(workspaceRoot, stdoutFileName), result.stdoutText, error)
        && writeTextArtifact(logPath(workspaceRoot, stderrFileName), result.stderrText, error);
}

bool TestgridArtifactService::writePhaseSummary(const QString& workspaceRoot,
                                                const QString& phase,
                                                const QVector<TestgridRow>& rows,
                                                QString* absolutePath,
                                                QString* error)
{
    QStringList lines;
    lines << QStringLiteral("module run pass fail pass_rate");
    for (const TestgridRow& row : rows)
    {
        lines << QStringLiteral("%1 %2 %3 %4 %5")
                .arg(row.module, row.runCount, row.passCount, row.failCount, row.passRate);
    }

    const QString path = verificationPath(workspaceRoot, QStringLiteral("testgrid_%1.txt").arg(phase));
    if (!writeTextArtifact(path, lines.join(QLatin1Char('\n')), error))
    {
        return false;
    }
    if (absolutePath != nullptr)
    {
        *absolutePath = path;
    }
    return true;
}

QVector<TestgridRow> TestgridArtifactService::readPhaseRows(const QString& workspaceRoot, const QString& phase)
{
    return VerificationResultParser::parseTestgridText(
        readTextArtifact(verificationPath(workspaceRoot, QStringLiteral("testgrid_%1.txt").arg(phase))));
}
} // namespace occtdebug
