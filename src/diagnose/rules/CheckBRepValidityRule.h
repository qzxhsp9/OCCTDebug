#pragma once

#include "diagnose/IDiagnosticRule.h"

class CheckBRepValidityRule final : public IDiagnosticRule
{
public:
    std::string id() const override { return "R401"; }
    std::string name() const override { return "BRepCheck validity"; }
    ProblemCategory category() const override { return ProblemCategory::Topology; }

    bool isApplicable(const ProblemContext& context, const ShapeDocument& document) const override;

    std::vector<DiagnosticFinding> run(
        const ProblemContext& context,
        const ShapeDocument& document) const override;
};
