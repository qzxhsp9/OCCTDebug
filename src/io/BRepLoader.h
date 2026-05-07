#pragma once

#include <QString>
#include <TopoDS_Shape.hxx>

struct BRepLoadResult
{
    bool ok = false;
    QString errorMessage;
    TopoDS_Shape shape;
};

/// Loads `.brep` via `BRepTools::Read`, or `.stp` / `.step` via `STEPControl_Reader`.
class BRepLoader
{
public:
    static BRepLoadResult loadFile(const QString& filePath);
};
