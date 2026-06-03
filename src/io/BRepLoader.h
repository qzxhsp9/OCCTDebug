#pragma once

#include <QString>
#include <TopoDS_Shape.hxx>

struct StepAssemblyInspectResult
{
    bool ok = false;
    QString errorMessage;

    bool isStep = false;
    bool stepStructureRead = false;
    bool hasAssembly = false;
    int assemblyCount = 0;
    int componentCount = 0;
    int freeShapeCount = 0;
};

struct BRepLoadResult
{
    bool ok = false;
    QString errorMessage;
    TopoDS_Shape shape;

    bool isStep = false;
    bool stepStructureRead = false;
    bool hasAssembly = false;
    int assemblyCount = 0;
    int componentCount = 0;
    int freeShapeCount = 0;
};

/// Loads `.brep` via `BRepTools::Read`, or `.stp` / `.step` via `STEPControl_Reader`.
class BRepLoader
{
public:
    static BRepLoadResult loadFile(const QString& filePath);
    static StepAssemblyInspectResult inspectStepAssembly(const QString& filePath);
};
