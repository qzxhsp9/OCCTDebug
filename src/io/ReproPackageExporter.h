#pragma once

#include "core/DiagnosticFinding.h"
#include "core/ProblemContext.h"

#include <QString>
#include <vector>

/// Writes a minimal folder: `case/input.brep`, `debug.occtdbg`, `README.txt` (Milestone 4).
class ReproPackageExporter
{
public:
    static bool exportMinimalPackage(
        const QString& packageDir,
        const ProblemContext& problem,
        const QString& primaryBrepAbsolute,
        const std::vector<DiagnosticFinding>& findings,
        QString* errorMessage);
};
