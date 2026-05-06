#include "diagnose/rules/CheckToleranceRule.h"

#include "core/ShapeKind.h"

#include <algorithm>
#include <sstream>

namespace
{
bool isTolShape(ShapeKind k)
{
    return k == ShapeKind::Vertex || k == ShapeKind::Edge || k == ShapeKind::Face;
}
} // namespace

bool CheckToleranceRule::isApplicable(const ProblemContext&, const ShapeDocument& document) const
{
    return !document.RootShape().IsNull();
}

std::vector<DiagnosticFinding> CheckToleranceRule::run(
    const ProblemContext&,
    const ShapeDocument& document) const
{
    std::vector<DiagnosticFinding> out;
    std::vector<double> tols;
    for (const ShapeNode& n : document.Nodes())
    {
        if (!isTolShape(n.kind))
        {
            continue;
        }
        if (n.tolerance > 0.0)
        {
            tols.push_back(n.tolerance);
        }
    }
    if (tols.empty())
    {
        return out;
    }

    std::sort(tols.begin(), tols.end());
    const double median = tols[tols.size() / 2];

    constexpr double kHard = 1e-2;
    constexpr double kSoftFactor = 50.0;

    for (const ShapeNode& n : document.Nodes())
    {
        if (!isTolShape(n.kind) || n.tolerance <= 0.0)
        {
            continue;
        }
        const bool hard = n.tolerance > kHard;
        const bool soft = (median > 0.0) && (n.tolerance > median * kSoftFactor);
        if (!hard && !soft)
        {
            continue;
        }
        DiagnosticFinding f;
        f.ruleId = id();
        f.severity = hard ? DiagnosticSeverity::Error : DiagnosticSeverity::Warning;
        f.title = hard ? "Tolerance unusually large" : "Tolerance larger than typical in this model";
        std::ostringstream desc;
        desc << "Shape id " << n.id << " tolerance=" << n.tolerance << " (median=" << median << ")";
        f.description = desc.str();
        f.relatedShapeIds.push_back(n.id);
        f.evidence.push_back(desc.str());
        f.possibleCauses.push_back("Dirty import, boolean fuzziness, or local shape healing");
        f.suggestions.push_back("Compare with adjacent entities; review boolean fuzzy value");
        out.push_back(std::move(f));
    }

    return out;
}
