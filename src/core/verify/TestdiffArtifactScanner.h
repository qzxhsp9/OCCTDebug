#pragma once

#include <QJsonObject>
#include <QString>

namespace occtdebug
{
class TestdiffArtifactScanner
{
public:
    static QJsonObject buildManifest(const QString& summaryPath,
                                     const QString& workspaceRoot,
                                     const QString& commandStdout = {},
                                     const QString& commandStderr = {},
                                     int entriesCount = 0,
                                     int changedCount = 0,
                                     int failedCount = 0);
};
} // namespace occtdebug
