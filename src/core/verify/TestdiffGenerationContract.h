#pragma once

#include <QJsonObject>
#include <QString>

namespace occtdebug
{
class TestdiffGenerationContract
{
public:
    static QString outputRoot();
    static QString caseManifestField();
    static QString sidecarSuffix();
    static QJsonObject build();
};
} // namespace occtdebug
