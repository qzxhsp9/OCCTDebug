#pragma once

#include "core/DiagnosticFinding.h"
#include "core/ProblemContext.h"
#include "core/ShapeDocument.h"
#include "diagnose/RuleRegistry.h"

#include <vector>

class DiagnosticEngine
{
public:
    DiagnosticEngine();

    std::vector<DiagnosticFinding> diagnose(const ProblemContext& context, const ShapeDocument& document);

private:
    RuleRegistry m_registry;
};
