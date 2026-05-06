#include "diagnose/rules/CheckBRepValidityRule.h"

#include <BRepCheck_Analyzer.hxx>

bool CheckBRepValidityRule::isApplicable(const ProblemContext&, const ShapeDocument& document) const
{
    return !document.RootShape().IsNull();
}

std::vector<DiagnosticFinding> CheckBRepValidityRule::run(
    const ProblemContext&,
    const ShapeDocument& document) const
{
    std::vector<DiagnosticFinding> out;
    BRepCheck_Analyzer analyzer(document.RootShape(), true);
    if (analyzer.IsValid())
    {
        return out;
    }

    DiagnosticFinding f;
    f.ruleId = id();
    f.severity = DiagnosticSeverity::Error;
    f.title = "BRepCheck_Analyzer reports invalid shape";
    f.description = "At least one sub-shape failed geometric/topologic checks.";
    f.evidence.push_back("BRepCheck_Analyzer::IsValid() == false");
    f.possibleCauses.push_back("Invalid curves on surface, inconsistent pcurves, bad tolerances");
    f.suggestions.push_back("Inspect sub-shapes with BRepCheck listing; try ShapeFix_* workflows");
    out.push_back(std::move(f));
    return out;
}
