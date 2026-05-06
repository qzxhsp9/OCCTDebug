#pragma once

#include "core/DiagnosticFinding.h"
#include "core/ProblemContext.h"
#include "core/ShapeDocument.h"

#include <QString>
#include <vector>

class MarkdownReportExporter
{
public:
    static QString exportReport(
        const ProblemContext& context,
        const ShapeDocument& document,
        const std::vector<DiagnosticFinding>& findings);
};
