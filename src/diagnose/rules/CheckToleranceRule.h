#pragma once

#include "diagnose/IDiagnosticRule.h"

class CheckToleranceRule final : public IDiagnosticRule
{
public:
    std::string id() const override { return "R301"; }
    std::string name() const override { return "Tolerance outliers"; }
    ProblemCategory category() const override { return ProblemCategory::Tolerance; }

    bool isApplicable(const ProblemContext& context, const ShapeDocument& document) const override;

    std::vector<DiagnosticFinding> run(
        const ProblemContext& context,
        const ShapeDocument& document) const override;
};
