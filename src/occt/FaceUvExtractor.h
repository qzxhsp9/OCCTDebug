#pragma once

#include <TopoDS_Face.hxx>

#include <string>
#include <utility>
#include <vector>

/// One polyline in the face (u,v) parameter plane (typically one wire).
struct FaceUvPolyline
{
    std::vector<std::pair<double, double>> points;
    bool closed = false;
    bool outerWire = false;
};

/// Sample pcurves of each wire on `face` into UV polylines (for 2D debug view).
bool OcctExtractFaceUvPolylines(const TopoDS_Face& face, std::vector<FaceUvPolyline>& out, std::string* errorOut);
