#pragma once

#include <QJsonObject>

namespace occtdebug
{
class TestdiffGenerationPolicy
{
public:
    static QJsonObject build(const QJsonObject& artifactIndex, const QJsonObject& artifactAnalysis);
    static QJsonObject build(
        const QJsonObject& artifactIndex,
        const QJsonObject& artifactAnalysis,
        const QJsonObject& generationConfig);
};
} // namespace occtdebug
