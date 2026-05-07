#pragma once

#include <TopoDS_Shape.hxx>

#include <QString>

/// Short HTML fragment (Face surface type / Edge curve type / Vertex coordinates) for the property panel.
QString OcctGeometrySummaryHtml(const TopoDS_Shape& shape);
