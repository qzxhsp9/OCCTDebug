#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace occtdebug
{
struct TestdiffRunnerImportResult
{
    bool success = false;
    QString error;
    int copiedFiles = 0;
    QStringList importedDirectories;
    QJsonObject manifest;
};

class TestdiffRunnerAdapter
{
public:
    static TestdiffRunnerImportResult importOutput(const QString& workspaceRoot,
                                                   const QString& runnerOutputRoot,
                                                   const QString& summaryPath = {},
                                                   const QString& commandStdout = {},
                                                   const QString& commandStderr = {},
                                                   int entriesCount = 0,
                                                   int changedCount = 0,
                                                   int failedCount = 0);
};
} // namespace occtdebug
