#include "diagnose/rules/CheckSameParameterRule.h"

#include <BRep_Tool.hxx>
#include <TopAbs.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

bool CheckSameParameterRule::isApplicable(const ProblemContext&, const ShapeDocument& document) const
{
    return !document.RootShape().IsNull();
}

std::vector<DiagnosticFinding> CheckSameParameterRule::run(
    const ProblemContext&,
    const ShapeDocument& document) const
{
    std::vector<DiagnosticFinding> out;
    TopTools_IndexedMapOfShape edgeMap;
    TopExp::MapShapes(document.RootShape(), TopAbs_EDGE, edgeMap);

    constexpr int kMaxFindings = 40;
    int count = 0;
    for (int i = 1; i <= edgeMap.Extent(); ++i)
    {
        const TopoDS_Edge& edge = TopoDS::Edge(edgeMap(i));
        if (!BRep_Tool::SameParameter(edge))
        {
            if (count >= kMaxFindings)
            {
                DiagnosticFinding cap;
                cap.ruleId = id();
                cap.severity = DiagnosticSeverity::Info;
                cap.title = "Additional SameParameter issues omitted";
                cap.description = "Too many edges flagged; showing first "
                    + std::to_string(kMaxFindings) + " only.";
                out.push_back(std::move(cap));
                break;
            }
            ++count;
            DiagnosticFinding f;
            f.ruleId = id();
            f.severity = DiagnosticSeverity::Warning;
            f.title = "Edge SameParameter is false";
            f.description = "BRep_Tool::SameParameter(edge) is false; 3D/pcurve consistency may be unchecked.";
            f.evidence.push_back("Edge index in map: " + std::to_string(i));
            f.possibleCauses.push_back("SameParameter not computed or inconsistent parameterization");
            f.suggestions.push_back("Run BRepLib::SameParameter(edge, tolerance) and re-check");
            out.push_back(std::move(f));
        }
    }
    return out;
}
