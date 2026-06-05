#pragma once

#include "core/case/CaseManifest.h"

#include <QJsonObject>
#include <QString>

namespace occtdebug
{
class EvidenceBundleWriter
{
public:
    static QJsonObject buildBundle(const CaseManifest& manifest, const QString& caseRoot);
    static bool writeBundle(const CaseManifest& manifest, const QString& caseRoot, const QString& outputPath, QString* error = nullptr);
};
} // namespace occtdebug
