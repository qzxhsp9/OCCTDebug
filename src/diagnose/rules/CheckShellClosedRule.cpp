#include "diagnose/rules/CheckShellClosedRule.h"

#include "core/ShapeKind.h"

bool CheckShellClosedRule::isApplicable(const ProblemContext&, const ShapeDocument& document) const
{
    return !document.RootShape().IsNull();
}

std::vector<DiagnosticFinding> CheckShellClosedRule::run(
    const ProblemContext&,
    const ShapeDocument& document) const
{
    std::vector<DiagnosticFinding> out;
    for (const ShapeNode& n : document.Nodes())
    {
        if (n.kind != ShapeKind::Shell || n.shape.IsNull())
        {
            continue;
        }
        if (!n.isClosed)
        {
            DiagnosticFinding f;
            f.ruleId = id();
            f.severity = DiagnosticSeverity::Warning;
            f.title = "Shell is not closed";
            f.description = "Shell has free boundaries according to BRep_Tool::IsClosed.";
            f.relatedShapeIds.push_back(n.id);
            f.evidence.push_back("Shape id " + std::to_string(n.id) + " (Shell)");
            f.possibleCauses.push_back("Missing faces, gaps, or non-manifold sheet");
            f.suggestions.push_back("Inspect boundary edges; run BRepCheck on the shell");
            out.push_back(std::move(f));
        }
    }
    return out;
}
