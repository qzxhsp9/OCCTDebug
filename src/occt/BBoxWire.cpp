#include "occt/BBoxWire.h"

#include <BRepBndLib.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <Bnd_Box.hxx>
#include <Precision.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Iterator.hxx>

#include <cmath>

namespace
{
void addEdgeIfOk(BRep_Builder& builder, TopoDS_Compound& compound, const gp_Pnt& a, const gp_Pnt& b)
{
    BRepBuilderAPI_MakeEdge mk(a, b);
    if (!mk.IsDone())
    {
        return;
    }
    const TopoDS_Edge e = mk.Edge();
    if (!e.IsNull())
    {
        builder.Add(compound, e);
    }
}
} // namespace

TopoDS_Shape OcctBuildBoundingBoxWire(const TopoDS_Shape& shape)
{
    if (shape.IsNull())
    {
        return TopoDS_Shape();
    }

    Bnd_Box box;
    BRepBndLib::Add(shape, box, Standard_False);
    if (box.IsVoid())
    {
        return TopoDS_Shape();
    }

    Standard_Real x0 = 0;
    Standard_Real y0 = 0;
    Standard_Real z0 = 0;
    Standard_Real x1 = 0;
    Standard_Real y1 = 0;
    Standard_Real z1 = 0;
    box.Get(x0, y0, z0, x1, y1, z1);

    const Standard_Real eps = Precision::Confusion() * 100.0;
    if (std::fabs(x1 - x0) < eps)
    {
        x0 -= eps;
        x1 += eps;
    }
    if (std::fabs(y1 - y0) < eps)
    {
        y0 -= eps;
        y1 += eps;
    }
    if (std::fabs(z1 - z0) < eps)
    {
        z0 -= eps;
        z1 += eps;
    }

    const gp_Pnt p000(x0, y0, z0);
    const gp_Pnt p100(x1, y0, z0);
    const gp_Pnt p110(x1, y1, z0);
    const gp_Pnt p010(x0, y1, z0);
    const gp_Pnt p001(x0, y0, z1);
    const gp_Pnt p101(x1, y0, z1);
    const gp_Pnt p111(x1, y1, z1);
    const gp_Pnt p011(x0, y1, z1);

    BRep_Builder b;
    TopoDS_Compound comp;
    b.MakeCompound(comp);

    addEdgeIfOk(b, comp, p000, p100);
    addEdgeIfOk(b, comp, p100, p110);
    addEdgeIfOk(b, comp, p110, p010);
    addEdgeIfOk(b, comp, p010, p000);

    addEdgeIfOk(b, comp, p001, p101);
    addEdgeIfOk(b, comp, p101, p111);
    addEdgeIfOk(b, comp, p111, p011);
    addEdgeIfOk(b, comp, p011, p001);

    addEdgeIfOk(b, comp, p000, p001);
    addEdgeIfOk(b, comp, p100, p101);
    addEdgeIfOk(b, comp, p110, p111);
    addEdgeIfOk(b, comp, p010, p011);

    TopoDS_Iterator it(comp);
    if (!it.More())
    {
        return TopoDS_Shape();
    }
    return comp;
}
