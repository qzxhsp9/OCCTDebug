#pragma once

#include <QJsonArray>
#include <QJsonObject>

namespace occtdebug
{
class TestdiffArtifactIndex
{
public:
    static QJsonObject build(const QJsonArray& artifactFiles);
};
} // namespace occtdebug
