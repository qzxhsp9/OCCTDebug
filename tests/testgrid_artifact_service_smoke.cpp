#include "core/verify/TestgridArtifactService.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTextStream>

namespace
{
bool expect(bool condition, const char* message)
{
    if (!condition)
    {
        QTextStream(stderr) << message << "\n";
    }
    return condition;
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir temp;
    if (!expect(temp.isValid(), "temporary directory is invalid"))
    {
        return 1;
    }

    QString error;
    if (!expect(occtdebug::TestgridArtifactService::ensureWorkspaceDirectories(temp.path(), &error), "directory creation failed"))
    {
        QTextStream(stderr) << error << "\n";
        return 2;
    }

    occtdebug::CommandResult result;
    result.stdoutText = QStringLiteral("stdout smoke");
    result.stderrText = QStringLiteral("stderr smoke");
    if (!expect(occtdebug::TestgridArtifactService::writeCommandLogs(
            temp.path(),
            QStringLiteral("testgrid.stdout.log"),
            QStringLiteral("testgrid.stderr.log"),
            result,
            &error), "command logs write failed"))
    {
        QTextStream(stderr) << error << "\n";
        return 3;
    }

    const QString stdoutPath = occtdebug::TestgridArtifactService::logPath(temp.path(), QStringLiteral("testgrid.stdout.log"));
    if (!expect(QFileInfo::exists(stdoutPath), "stdout log does not exist")
        || !expect(occtdebug::TestgridArtifactService::readTextArtifact(stdoutPath).contains(QStringLiteral("stdout smoke")), "stdout log content mismatch"))
    {
        return 4;
    }

    const QVector<occtdebug::TestgridRow> rows {
        {QStringLiteral("Modeling"), QStringLiteral("2"), QStringLiteral("1"), QStringLiteral("1"), QStringLiteral("50%")},
    };
    QString summaryPath;
    if (!expect(occtdebug::TestgridArtifactService::writePhaseSummary(temp.path(), QStringLiteral("before"), rows, &summaryPath, &error), "phase summary write failed"))
    {
        QTextStream(stderr) << error << "\n";
        return 5;
    }
    const QVector<occtdebug::TestgridRow> parsed = occtdebug::TestgridArtifactService::readPhaseRows(temp.path(), QStringLiteral("before"));
    if (!expect(parsed.size() == 1, "phase summary row count mismatch")
        || !expect(parsed.first().module == QStringLiteral("Modeling"), "phase summary module mismatch")
        || !expect(occtdebug::TestgridArtifactService::verificationRelativePath(QStringLiteral("testgrid_before.txt")) == QStringLiteral("verification/testgrid_before.txt"), "verification relative path mismatch"))
    {
        return 6;
    }

    if (!expect(occtdebug::TestgridArtifactService::writeJsonArtifact(
            occtdebug::TestgridArtifactService::artifactPath(temp.path(), QStringLiteral("testgrid_result.json")),
            QJsonObject {{QStringLiteral("schema_version"), 1}},
            &error), "json artifact write failed"))
    {
        QTextStream(stderr) << error << "\n";
        return 7;
    }

    QTextStream(stdout) << "TESTGRID_ARTIFACT_SERVICE_SMOKE_OK\n";
    return 0;
}
