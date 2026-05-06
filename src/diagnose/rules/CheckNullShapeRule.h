#pragma once

#include "diagnose/IDiagnosticRule.h"

class CheckNullShapeRule final : public IDiagnosticRule
{
public:
    std::string id() const override { return "R001"; }
    std::string name() const override { return "Shape is null"; }
    ProblemCategory category() const override { return ProblemCategory::Topology; }

    bool isApplicable(const ProblemContext& context, const ShapeDocument& document) const override;

    std::vector<DiagnosticFinding> run(
        const ProblemContext& context,
        const ShapeDocument& document) const override;
};
