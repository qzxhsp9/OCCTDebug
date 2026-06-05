#pragma once

#include "core/case/CaseManifest.h"

#include <QJsonObject>
#include <QString>

namespace occtdebug
{
class VerificationReportWriter
{
public:
    static QJsonObject buildReport(const CaseManifest& manifest, const QString& caseRoot);
    static bool writeReport(const CaseManifest& manifest,
                            const QString& caseRoot,
                            const QString& markdownPath,
                            const QString& jsonPath,
                            QString* error = nullptr);
};
} // namespace occtdebug
