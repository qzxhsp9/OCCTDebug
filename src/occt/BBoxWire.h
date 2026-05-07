#pragma once

#include <TopoDS_Shape.hxx>

/// Builds a compound of 12 edges for the axis-aligned bounding box of `shape` (empty if void).
TopoDS_Shape OcctBuildBoundingBoxWire(const TopoDS_Shape& shape);
