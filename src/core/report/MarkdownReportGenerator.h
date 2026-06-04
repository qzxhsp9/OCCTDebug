#pragma once

#include "core/case/CaseManifest.h"

#include <QString>

namespace occtdebug
{
class MarkdownReportGenerator
{
public:
    static bool writeReproReport(const CaseManifest& manifest, const QString& filePath, QString* error = nullptr);
};
} // namespace occtdebug
