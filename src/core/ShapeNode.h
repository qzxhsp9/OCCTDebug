#pragma once

#include "core/ShapeKind.h"

#include <Bnd_Box.hxx>
#include <TopoDS_Shape.hxx>

#include <string>
#include <vector>

struct ShapeNode
{
    int id = -1;
    int parentId = -1;

    ShapeKind kind = ShapeKind::Unknown;
    std::string name;

    TopoDS_Shape shape;

    double tolerance = 0.0;

    bool isNull = false;
    bool isClosed = false;
    bool isValid = true;

    Bnd_Box bbox;

    std::vector<int> children;
};
