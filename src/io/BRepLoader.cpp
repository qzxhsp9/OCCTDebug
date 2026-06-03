#include "io/BRepLoader.h"

#include <QFileInfo>

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <Message_ProgressRange.hxx>
#include <STEPControl_Reader.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>

namespace
{
void collectAssemblyInfo(const TDF_Label& label, StepAssemblyInspectResult& out)
{
    if (!XCAFDoc_ShapeTool::IsShape(label))
    {
        return;
    }

    if (!XCAFDoc_ShapeTool::IsAssembly(label))
    {
        return;
    }

    ++out.assemblyCount;
    out.hasAssembly = true;

    TDF_LabelSequence components;
    XCAFDoc_ShapeTool::GetComponents(label, components, Standard_False);
    out.componentCount += components.Length();

    for (Standard_Integer i = 1; i <= components.Length(); ++i)
    {
        TDF_Label referred;
        const TDF_Label component = components.Value(i);
        if (XCAFDoc_ShapeTool::GetReferredShape(component, referred))
        {
            collectAssemblyInfo(referred, out);
        }
        else
        {
            collectAssemblyInfo(component, out);
        }
    }
}

StepAssemblyInspectResult inspectStepAssemblyStructure(const QString& filePath)
{
    StepAssemblyInspectResult out;
    out.isStep = true;

    Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
    Handle(TDocStd_Document) doc;
    app->NewDocument(TCollection_ExtendedString("MDTV-XCAF"), doc);

    STEPCAFControl_Reader reader;
    const IFSelect_ReturnStatus stat = reader.ReadFile(filePath.toUtf8().constData());
    if (stat != IFSelect_RetDone)
    {
        out.errorMessage = QStringLiteral("STEP structure read failed (status %1): %2")
                               .arg(static_cast<int>(stat))
                               .arg(filePath);
        app->Close(doc);
        return out;
    }

    if (!reader.Transfer(doc, Message_ProgressRange()))
    {
        out.errorMessage = QStringLiteral("STEP structure transfer failed: %1").arg(filePath);
        app->Close(doc);
        return out;
    }

    const Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
    if (shapeTool.IsNull())
    {
        out.errorMessage = QStringLiteral("STEP structure has no XCAF shape tool: %1").arg(filePath);
        app->Close(doc);
        return out;
    }

    out.ok = true;
    out.stepStructureRead = true;

    TDF_LabelSequence freeShapes;
    shapeTool->GetFreeShapes(freeShapes);
    out.freeShapeCount = freeShapes.Length();

    for (Standard_Integer i = 1; i <= freeShapes.Length(); ++i)
    {
        collectAssemblyInfo(freeShapes.Value(i), out);
    }

    app->Close(doc);
    return out;
}

void copyStepAssemblyInfo(const StepAssemblyInspectResult& src, BRepLoadResult& dst)
{
    dst.isStep = src.isStep;
    dst.stepStructureRead = src.stepStructureRead;
    dst.hasAssembly = src.hasAssembly;
    dst.assemblyCount = src.assemblyCount;
    dst.componentCount = src.componentCount;
    dst.freeShapeCount = src.freeShapeCount;
}

BRepLoadResult loadBrep(const QString& filePath)
{
    BRepLoadResult out;
    BRep_Builder builder;
    TopoDS_Shape shape;
    const Standard_Boolean readOk = BRepTools::Read(shape, filePath.toUtf8().constData(), builder);
    if (!readOk || shape.IsNull())
    {
        out.ok = false;
        out.errorMessage =
            QStringLiteral("BRepTools::Read failed or shape is null: %1").arg(filePath);
        return out;
    }
    out.ok = true;
    out.shape = shape;
    return out;
}

BRepLoadResult loadStep(const QString& filePath)
{
    BRepLoadResult out;
    out.isStep = true;
    STEPControl_Reader reader;
    const IFSelect_ReturnStatus stat = reader.ReadFile(filePath.toUtf8().constData());
    if (stat != IFSelect_RetDone)
    {
        out.ok = false;
        out.errorMessage = QStringLiteral("STEP read failed (status %1): %2")
                               .arg(static_cast<int>(stat))
                               .arg(filePath);
        return out;
    }

    reader.TransferRoots(Message_ProgressRange());
    const TopoDS_Shape shape = reader.OneShape();
    if (shape.IsNull())
    {
        out.ok = false;
        out.errorMessage =
            QStringLiteral("STEP transfer produced no shape (empty file?): %1").arg(filePath);
        return out;
    }

    out.ok = true;
    out.shape = shape;
    copyStepAssemblyInfo(inspectStepAssemblyStructure(filePath), out);
    return out;
}
} // namespace

BRepLoadResult BRepLoader::loadFile(const QString& filePath)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == QStringLiteral("stp") || suffix == QStringLiteral("step"))
    {
        return loadStep(filePath);
    }
    return loadBrep(filePath);
}

StepAssemblyInspectResult BRepLoader::inspectStepAssembly(const QString& filePath)
{
    StepAssemblyInspectResult out;
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix != QStringLiteral("stp") && suffix != QStringLiteral("step"))
    {
        out.ok = false;
        out.isStep = false;
        out.errorMessage = QStringLiteral("Not a STEP/STP file: %1").arg(filePath);
        return out;
    }

    return inspectStepAssemblyStructure(filePath);
}
