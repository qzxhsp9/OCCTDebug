#include "io/BRepLoader.h"

#include <BRep_Builder.hxx>
#include <BRepTools.hxx>

BRepLoadResult BRepLoader::loadFile(const QString& filePath)
{
    BRepLoadResult out;
    BRep_Builder builder;
    TopoDS_Shape shape;
    const Standard_Boolean readOk = BRepTools::Read(shape, filePath.toUtf8().constData(), builder);
    if (!readOk || shape.IsNull())
    {
        out.ok = false;
        out.errorMessage = QStringLiteral("BRepTools::Read failed or shape is null: %1").arg(filePath);
        return out;
    }
    out.ok = true;
    out.shape = shape;
    return out;
}
