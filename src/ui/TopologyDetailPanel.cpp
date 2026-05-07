#include "ui/TopologyDetailPanel.h"

#include "core/ShapeDocument.h"
#include "core/ShapeKind.h"
#include "core/ShapeNode.h"
#include "occt/FaceUvExtractor.h"
#include "ui/EdgeSchematicWidget.h"
#include "ui/FaceUvCanvasWidget.h"

#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <GeomAbs_CurveType.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>

#include <QLabel>
#include <QStackedWidget>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace
{
const char* curveTypeName(GeomAbs_CurveType t)
{
    switch (t)
    {
    case GeomAbs_Line:
        return "Line";
    case GeomAbs_Circle:
        return "Circle";
    case GeomAbs_Ellipse:
        return "Ellipse";
    case GeomAbs_BSplineCurve:
        return "BSplineCurve";
    case GeomAbs_BezierCurve:
        return "BezierCurve";
    default:
        return "OtherCurve";
    }
}
} // namespace

TopologyDetailPanel::TopologyDetailPanel(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("TopologyDetailPanel"));
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(4, 4, 4, 4);
    m_title = new QLabel(tr("Select a Face, Edge, or Vertex in the shape tree."), this);
    m_title->setWordWrap(true);
    lay->addWidget(m_title);

    m_stack = new QStackedWidget(this);
    lay->addWidget(m_stack, 1);

    m_placeholder = new QLabel(
        tr("No detail - choose Face (UV / pcurve), Edge (tolerance schematic), or Vertex (adjacent "
           "edges)."),
        this);
    m_placeholder->setWordWrap(true);
    m_placeholder->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_stack->addWidget(m_placeholder);

    auto* facePage = new QWidget(m_stack);
    auto* faceLay = new QVBoxLayout(facePage);
    faceLay->setContentsMargins(0, 0, 0, 0);
    m_faceCanvas = new FaceUvCanvasWidget(facePage);
    m_faceCanvas->setMinimumHeight(200);
    faceLay->addWidget(m_faceCanvas, 1);
    m_stack->addWidget(facePage);

    auto* edgePage = new QWidget(m_stack);
    auto* edgeLay = new QVBoxLayout(edgePage);
    edgeLay->setContentsMargins(0, 0, 0, 0);
    m_edgeInfo = new QTextBrowser(edgePage);
    m_edgeInfo->setReadOnly(true);
    m_edgeInfo->setMinimumHeight(80);
    m_edgeSchematic = new EdgeSchematicWidget(edgePage);
    edgeLay->addWidget(m_edgeInfo);
    edgeLay->addWidget(m_edgeSchematic);
    m_stack->addWidget(edgePage);

    auto* vertexPage = new QWidget(m_stack);
    auto* vLay = new QVBoxLayout(vertexPage);
    vLay->setContentsMargins(0, 0, 0, 0);
    m_vertexInfo = new QTextBrowser(vertexPage);
    m_vertexInfo->setReadOnly(true);
    vLay->addWidget(m_vertexInfo);
    m_stack->addWidget(vertexPage);
}

void TopologyDetailPanel::inspect(const ShapeDocument& document, int shapeId)
{
    if (shapeId < 0)
    {
        m_title->setText(tr("Topology detail"));
        m_stack->setCurrentWidget(m_placeholder);
        return;
    }

    const ShapeNode* node = document.FindNode(shapeId);
    if (!node || node->shape.IsNull())
    {
        m_stack->setCurrentWidget(m_placeholder);
        return;
    }

    switch (node->kind)
    {
    case ShapeKind::Face: {
        const TopoDS_Face face = TopoDS::Face(node->shape);
        std::vector<FaceUvPolyline> pls;
        std::string err;
        if (OcctExtractFaceUvPolylines(face, pls, &err))
        {
            m_faceCanvas->setPolylines(pls);
            m_stack->setCurrentIndex(1);
            m_title->setText(tr("Face #%1 - (u,v) domain").arg(shapeId));
        }
        else
        {
            m_placeholder->setText(
                tr("UV extraction failed: %1").arg(QString::fromStdString(err)));
            m_stack->setCurrentWidget(m_placeholder);
        }
        return;
    }
    case ShapeKind::Edge: {
        const TopoDS_Edge edge = TopoDS::Edge(node->shape);
        BRepAdaptor_Curve c3d(edge);
        const double t0 = c3d.FirstParameter();
        const double t1 = c3d.LastParameter();
        const double edgeTol = BRep_Tool::Tolerance(edge);

        TopoDS_Vertex va;
        TopoDS_Vertex vb;
        TopExp::Vertices(edge, va, vb);
        const double tolA = va.IsNull() ? 0.0 : BRep_Tool::Tolerance(va);
        const double tolB = vb.IsNull() ? 0.0 : BRep_Tool::Tolerance(vb);

        QString html;
        html += QStringLiteral("<b>Edge geometry</b><br/>");
        html += QStringLiteral("Curve type: %1<br/>").arg(QString::fromLatin1(curveTypeName(c3d.GetType())));
        html += QStringLiteral("Edge tolerance: %1<br/>").arg(edgeTol, 0, 'g', 10);
        html += QStringLiteral("T range: [%1, %2]<br/>").arg(t0, 0, 'g', 12).arg(t1, 0, 'g', 12);
        html += QStringLiteral("Vertex tolerances: %1 / %2<br/>")
                    .arg(tolA, 0, 'g', 10)
                    .arg(tolB, 0, 'g', 10);
        m_edgeInfo->setHtml(html);
        m_edgeSchematic->setTolerances(tolA, tolB, edgeTol);
        m_stack->setCurrentIndex(2);
        m_title->setText(tr("Edge #%1 - t / tolerance schematic").arg(shapeId));
        return;
    }
    case ShapeKind::Vertex: {
        const TopoDS_Vertex vx = TopoDS::Vertex(node->shape);
        const double tolV = BRep_Tool::Tolerance(vx);
        const TopoDS_Shape root = document.RootShape();
        QString html;
        html += QStringLiteral("<b>Vertex</b><br/>");
        html += QStringLiteral("Tolerance: %1<br/><br/>").arg(tolV, 0, 'g', 10);
        html += QStringLiteral("<b>Adjacent edges</b> (topology scan on root shape)<br/><ul>");

        int count = 0;
        if (!root.IsNull())
        {
            for (TopExp_Explorer ex(root, TopAbs_EDGE); ex.More(); ex.Next())
            {
                const TopoDS_Edge e = TopoDS::Edge(ex.Current());
                TopoDS_Vertex a;
                TopoDS_Vertex b;
                TopExp::Vertices(e, a, b);
                if ((!a.IsNull() && a.IsSame(vx)) || (!b.IsNull() && b.IsSame(vx)))
                {
                    ++count;
                    const double te = BRep_Tool::Tolerance(e);
                    html += QStringLiteral("<li>#%1 - edge tolerance %2</li>")
                                .arg(count)
                                .arg(te, 0, 'g', 8);
                }
            }
        }
        html += QStringLiteral("</ul><p>Total adjacent edges: %1</p>").arg(count);
        html += QStringLiteral(
            "<p><i>Later: local 3D fan / star view of these edges.</i></p>");
        m_vertexInfo->setHtml(html);
        m_stack->setCurrentIndex(3);
        m_title->setText(tr("Vertex #%1 - adjacent edges").arg(shapeId));
        return;
    }
    default:
        m_placeholder->setText(
            tr("Topology detail is available for <b>Face</b>, <b>Edge</b>, and <b>Vertex</b> only."));
        m_stack->setCurrentWidget(m_placeholder);
        return;
    }
}
