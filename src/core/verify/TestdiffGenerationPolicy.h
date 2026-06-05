#pragma once

#include <QJsonObject>

namespace occtdebug
{
class TestdiffGenerationPolicy
{
public:
    static QJsonObject build(const QJsonObject& artifactIndex, const QJsonObject& artifactAnalysis);
};
} // namespace occtdebug
