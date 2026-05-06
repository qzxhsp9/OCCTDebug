#include "occt/ShapeInspector.h"

#include "core/ShapeKind.h"
#include "occt/OcctUtils.h"

#include <BRepBndLib.hxx>
#include <BRep_Tool.hxx>
#include <TopoDS_Iterator.hxx>

#include <sstream>
#include <string>

namespace
{
int visitShape(ShapeDocument& doc, const TopoDS_Shape& shape, int parentId)
{
    ShapeNode node;
    node.parentId = parentId;
    node.shape = shape;
    node.kind = ShapeKindFromTopAbs(shape.ShapeType());
    node.isNull = shape.IsNull();
    node.tolerance = OcctShapeTolerance(shape);

    Bnd_Box box;
    if (!shape.IsNull())
    {
        BRepBndLib::Add(shape, box);
    }
    node.bbox = box;

    if (!shape.IsNull())
    {
        if (shape.ShapeType() == TopAbs_WIRE || shape.ShapeType() == TopAbs_SHELL)
        {
            node.isClosed = BRep_Tool::IsClosed(shape);
        }
        else
        {
            node.isClosed = shape.Closed();
        }
    }

    const int myId = doc.AddNode(std::move(node));

    {
        ShapeNode* me = doc.FindNode(myId);
        if (me)
        {
            std::ostringstream oss;
            oss << '#' << myId << ' ' << ShapeKindDisplayName(me->kind);
            me->name = oss.str();
        }
    }

    for (TopoDS_Iterator it(shape); it.More(); it.Next())
    {
        const int childId = visitShape(doc, it.Value(), myId);
        ShapeNode* me = doc.FindNode(myId);
        if (me)
        {
            me->children.push_back(childId);
        }
    }

    return myId;
}
} // namespace

void ShapeInspector::BuildFromShape(ShapeDocument& doc, const TopoDS_Shape& root)
{
    doc.Clear();
    doc.SetRootShape(root);
    if (!root.IsNull())
    {
        visitShape(doc, root, -1);
    }
}
