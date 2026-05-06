#include "diagnose/rules/CheckWireClosedRule.h"

#include "core/ShapeKind.h"

bool CheckWireClosedRule::isApplicable(const ProblemContext&, const ShapeDocument& document) const
{
    return !document.RootShape().IsNull();
}

std::vector<DiagnosticFinding> CheckWireClosedRule::run(
    const ProblemContext&,
    const ShapeDocument& document) const
{
    std::vector<DiagnosticFinding> out;
    for (const ShapeNode& n : document.Nodes())
    {
        if (n.kind != ShapeKind::Wire || n.shape.IsNull())
        {
            continue;
        }
        if (!n.isClosed)
        {
            DiagnosticFinding f;
            f.ruleId = id();
            f.severity = DiagnosticSeverity::Warning;
            f.title = "Wire is not closed";
            f.description = "BRep_Tool::IsClosed returned false for a wire in the shape tree.";
            f.relatedShapeIds.push_back(n.id);
            f.evidence.push_back("Shape id " + std::to_string(n.id) + " (Wire)");
            f.possibleCauses.push_back("Trimming or import left an open contour");
            f.suggestions.push_back("Inspect adjacent edges and vertices; consider ShapeFix_Wire");
            out.push_back(std::move(f));
        }
    }
    return out;
}
