#pragma once

#include "core/ShapeDocument.h"

#include <TopoDS_Shape.hxx>

class ShapeInspector
{
public:
    /// Clears \p doc and fills it with a depth-first tree matching direct OCCT sub-shapes.
    static void BuildFromShape(ShapeDocument& doc, const TopoDS_Shape& root);
};
