#include "core/ShapeKind.h"

#include <TopAbs.hxx>

ShapeKind ShapeKindFromTopAbs(TopAbs_ShapeEnum t)
{
    switch (t)
    {
    case TopAbs_COMPOUND:
        return ShapeKind::Compound;
    case TopAbs_COMPSOLID:
        return ShapeKind::CompSolid;
    case TopAbs_SOLID:
        return ShapeKind::Solid;
    case TopAbs_SHELL:
        return ShapeKind::Shell;
    case TopAbs_FACE:
        return ShapeKind::Face;
    case TopAbs_WIRE:
        return ShapeKind::Wire;
    case TopAbs_EDGE:
        return ShapeKind::Edge;
    case TopAbs_VERTEX:
        return ShapeKind::Vertex;
    case TopAbs_SHAPE:
    default:
        return ShapeKind::Unknown;
    }
}
