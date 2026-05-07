#include "io/BRepLoader.h"

#include <QFileInfo>

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <Message_ProgressRange.hxx>
#include <STEPControl_Reader.hxx>

namespace
{
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
