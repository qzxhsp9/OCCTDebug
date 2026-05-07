#include "occt/GeometrySummary.h"

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRep_Tool.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>

namespace
{
const char* surfaceTypeName(GeomAbs_SurfaceType t)
{
    switch (t)
    {
    case GeomAbs_Plane:
        return "Plane";
    case GeomAbs_Cylinder:
        return "Cylinder";
    case GeomAbs_Cone:
        return "Cone";
    case GeomAbs_Sphere:
        return "Sphere";
    case GeomAbs_Torus:
        return "Torus";
    case GeomAbs_BezierSurface:
        return "BezierSurface";
    case GeomAbs_BSplineSurface:
        return "BSplineSurface";
    case GeomAbs_SurfaceOfRevolution:
        return "SurfaceOfRevolution";
    case GeomAbs_SurfaceOfExtrusion:
        return "SurfaceOfExtrusion";
    case GeomAbs_OffsetSurface:
        return "OffsetSurface";
    case GeomAbs_OtherSurface:
    default:
        return "OtherSurface";
    }
}

const char* curveTypeName(GeomAbs_CurveType t)
{
    switch (t)
    {
    case GeomAbs_Line:
        return "Line";
    case GeomAbs_Circle:
        return "Circle";
    case GeomAbs_Ellipse:
        return "Ellipse";
    case GeomAbs_Hyperbola:
        return "Hyperbola";
    case GeomAbs_Parabola:
        return "Parabola";
    case GeomAbs_BezierCurve:
        return "BezierCurve";
    case GeomAbs_BSplineCurve:
        return "BSplineCurve";
    case GeomAbs_OffsetCurve:
        return "OffsetCurve";
    case GeomAbs_OtherCurve:
    default:
        return "OtherCurve";
    }
}
} // namespace

QString OcctGeometrySummaryHtml(const TopoDS_Shape& shape)
{
    if (shape.IsNull())
    {
        return QStringLiteral("<b>Geometry</b><br/><i>null</i><br/>");
    }

    switch (shape.ShapeType())
    {
    case TopAbs_FACE: {
        BRepAdaptor_Surface surf(TopoDS::Face(shape));
        const GeomAbs_SurfaceType st = surf.GetType();
        QString html = QStringLiteral("<b>Geometry</b><br/>");
        html += QStringLiteral("Surface: %1<br/>").arg(QString::fromLatin1(surfaceTypeName(st)));
        html += QStringLiteral("U: [%1, %2] &nbsp; V: [%3, %4]<br/>")
                    .arg(surf.FirstUParameter(), 0, 'g', 8)
                    .arg(surf.LastUParameter(), 0, 'g', 8)
                    .arg(surf.FirstVParameter(), 0, 'g', 8)
                    .arg(surf.LastVParameter(), 0, 'g', 8);
        return html;
    }
    case TopAbs_EDGE: {
        BRepAdaptor_Curve crv(TopoDS::Edge(shape));
        const GeomAbs_CurveType ct = crv.GetType();
        QString html = QStringLiteral("<b>Geometry</b><br/>");
        html += QStringLiteral("Curve: %1<br/>").arg(QString::fromLatin1(curveTypeName(ct)));
        html += QStringLiteral("T: [%1, %2]<br/>")
                    .arg(crv.FirstParameter(), 0, 'g', 10)
                    .arg(crv.LastParameter(), 0, 'g', 10);
        return html;
    }
    case TopAbs_VERTEX: {
        const gp_Pnt p = BRep_Tool::Pnt(TopoDS::Vertex(shape));
        return QStringLiteral("<b>Geometry</b><br/>"
                              "3D point: (%1, %2, %3)<br/>")
            .arg(p.X(), 0, 'g', 12)
            .arg(p.Y(), 0, 'g', 12)
            .arg(p.Z(), 0, 'g', 12);
    }
    default:
        return QStringLiteral("<b>Geometry</b><br/>"
                              "<i>Select a Face, Edge, or Vertex for surface/curve details.</i><br/>");
    }
}
