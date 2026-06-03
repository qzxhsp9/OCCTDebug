#include "occt/FaceUvExtractor.h"

#include <BRepAdaptor_Curve2d.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <Geom2d_Curve.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>

#include <algorithm>
#include <cmath>

namespace
{
bool sampleEdgeUv(
    const TopoDS_Edge& edge,
    const TopoDS_Face& face,
    std::vector<std::pair<double, double>>& pts)
{
    double f2d = 0.0;
    double l2d = 0.0;
    const Handle(Geom2d_Curve) pc2d = BRep_Tool::CurveOnSurface(edge, face, f2d, l2d);
    if (pc2d.IsNull())
    {
        return false;
    }
    BRepAdaptor_Curve2d c2d(edge, face);
    const double t0 = c2d.FirstParameter();
    const double t1 = c2d.LastParameter();
    if (!(t1 > t0) && !(t1 < t0))
    {
        return true;
    }
    const int n = std::clamp(static_cast<int>(std::ceil(std::fabs(t1 - t0) * 32.0)), 8, 256);
    for (int i = 0; i <= n; ++i)
    {
        const double u = t0 + (t1 - t0) * (static_cast<double>(i) / static_cast<double>(n));
        const gp_Pnt2d p = c2d.Value(u);
        pts.push_back({p.X(), p.Y()});
    }
    return true;
}
} // namespace

bool OcctExtractFaceUvPolylines(const TopoDS_Face& face, std::vector<FaceUvPolyline>& out, std::string* errorOut)
{
    out.clear();
    if (face.IsNull())
    {
        if (errorOut != nullptr)
        {
            *errorOut = "Face is null.";
        }
        return false;
    }

    TopoDS_Wire outer = BRepTools::OuterWire(face);

    for (TopExp_Explorer wx(face, TopAbs_WIRE); wx.More(); wx.Next())
    {
        const TopoDS_Wire wire = TopoDS::Wire(wx.Current());
        FaceUvPolyline poly;
        poly.outerWire =
            (!outer.IsNull() && (wire.IsSame(outer) || wire.IsPartner(outer)));

        for (TopExp_Explorer ex(wire, TopAbs_EDGE); ex.More(); ex.Next())
        {
            const TopoDS_Edge edge = TopoDS::Edge(ex.Current());
            std::vector<std::pair<double, double>> seg;
            if (!sampleEdgeUv(edge, face, seg) || seg.empty())
            {
                continue;
            }
            poly.edgeBreaks.push_back(poly.points.size());
            if (!poly.points.empty())
            {
                const auto& a = poly.points.back();
                const auto& b = seg.front();
                if (std::hypot(a.first - b.first, a.second - b.second) < 1e-9 && seg.size() > 1)
                {
                    poly.points.insert(poly.points.end(), seg.begin() + 1, seg.end());
                }
                else
                {
                    poly.points.insert(poly.points.end(), seg.begin(), seg.end());
                }
            }
            else
            {
                poly.points.insert(poly.points.end(), seg.begin(), seg.end());
            }
        }

        if (poly.points.size() >= 2)
        {
            poly.edgeBreaks.push_back(poly.points.size());
            const auto& a = poly.points.front();
            const auto& b = poly.points.back();
            poly.closed = std::hypot(a.first - b.first, a.second - b.second)
                < 1e-7 * (std::fabs(a.first) + std::fabs(a.second) + 1.0);
            out.push_back(std::move(poly));
        }
    }

    if (out.empty() && errorOut != nullptr)
    {
        *errorOut = "No UV polylines (missing pcurves or degenerate wires).";
    }
    return !out.empty();
}
