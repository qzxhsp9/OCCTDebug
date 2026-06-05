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
    static QString configManifestField();
    static QString failureReportPath();
    static QString sidecarSuffix();
    static QJsonObject defaultConfig();
    static QJsonObject failureReportContract();
    static QJsonObject build();
};
} // namespace occtdebug
