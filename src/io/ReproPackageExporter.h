#pragma once

#include "core/DiagnosticFinding.h"
#include "core/ProblemContext.h"

#include <QString>
#include <vector>

/// Writes `case/<model file>`, `debug.occtdbg`, `README.txt` (Milestone 4).
class ReproPackageExporter
{
public:
    static bool exportMinimalPackage(
        const QString& packageDir,
        const ProblemContext& problem,
        const QString& primaryBrepAbsolute, ///< absolute path to primary `.brep` or `.stp`/`.step`
        const std::vector<DiagnosticFinding>& findings,
        QString* errorMessage);
};
