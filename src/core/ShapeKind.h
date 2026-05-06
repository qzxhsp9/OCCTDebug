#pragma once

#include <TopAbs_ShapeEnum.hxx>

enum class ShapeKind
{
    Unknown,
    Compound,
    CompSolid,
    Solid,
    Shell,
    Face,
    Wire,
    Edge,
    Vertex
};

ShapeKind ShapeKindFromTopAbs(TopAbs_ShapeEnum t);
