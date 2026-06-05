#pragma once

#include <QJsonObject>
#include <QString>

namespace occtdebug
{
class TestdiffArtifactAnalysis
{
public:
    static QJsonObject build(const QString& workspaceRoot, const QJsonObject& artifactIndex);
};
} // namespace occtdebug
