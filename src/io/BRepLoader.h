#pragma once

#include <QString>
#include <TopoDS_Shape.hxx>

struct BRepLoadResult
{
    bool ok = false;
    QString errorMessage;
    TopoDS_Shape shape;
};

class BRepLoader
{
public:
    static BRepLoadResult loadFile(const QString& filePath);
};
