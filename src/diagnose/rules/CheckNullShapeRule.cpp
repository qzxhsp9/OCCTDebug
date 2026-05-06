#include "diagnose/rules/CheckNullShapeRule.h"

bool CheckNullShapeRule::isApplicable(const ProblemContext&, const ShapeDocument&) const
{
    return true;
}

std::vector<DiagnosticFinding> CheckNullShapeRule::run(
    const ProblemContext&,
    const ShapeDocument& document) const
{
    std::vector<DiagnosticFinding> out;
    if (document.RootShape().IsNull())
    {
        DiagnosticFinding f;
        f.ruleId = id();
        f.severity = DiagnosticSeverity::Critical;
        f.title = "Root shape is null";
        f.description = "No valid TopoDS_Shape was loaded.";
        f.evidence.push_back("document.RootShape().IsNull() == true");
        f.possibleCauses.push_back("Empty or failed BREP read");
        f.suggestions.push_back("Verify file path and BREP contents");
        out.push_back(std::move(f));
    }
    return out;
}
