// Minimal OCCT link smoke test (CTest). Does not require Qt.
#include <BRepPrimAPI_MakeBox.hxx>
#include <TopoDS_Solid.hxx>

#include "occt/ShapeInspector.h"

int main()
{
    BRepPrimAPI_MakeBox mk(1.0, 2.0, 3.0);
    const TopoDS_Solid box = mk.Solid();
    ShapeDocument doc;
    ShapeInspector::BuildFromShape(doc, box);
    return doc.Nodes().empty() ? 1 : 0;
}
