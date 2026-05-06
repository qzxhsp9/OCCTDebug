#pragma once

#include "core/ShapeKind.h"

#include <BRep_Tool.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>

inline double OcctShapeTolerance(const TopoDS_Shape& shape)
{
    switch (shape.ShapeType())
    {
    case TopAbs_VERTEX:
        return BRep_Tool::Tolerance(TopoDS::Vertex(shape));
    case TopAbs_EDGE:
        return BRep_Tool::Tolerance(TopoDS::Edge(shape));
    case TopAbs_FACE:
        return BRep_Tool::Tolerance(TopoDS::Face(shape));
    default:
        return 0.0;
    }
}

inline const char* ShapeKindDisplayName(ShapeKind k)
{
    switch (k)
    {
    case ShapeKind::Compound:
        return "Compound";
    case ShapeKind::CompSolid:
        return "CompSolid";
    case ShapeKind::Solid:
        return "Solid";
    case ShapeKind::Shell:
        return "Shell";
    case ShapeKind::Face:
        return "Face";
    case ShapeKind::Wire:
        return "Wire";
    case ShapeKind::Edge:
        return "Edge";
    case ShapeKind::Vertex:
        return "Vertex";
    case ShapeKind::Unknown:
    default:
        return "Unknown";
    }
}
