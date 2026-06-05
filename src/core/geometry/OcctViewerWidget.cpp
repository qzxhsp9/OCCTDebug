#include "core/geometry/OcctViewerWidget.h"

#include "core/geometry/TopologySignature.h"

#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <BRep_Builder.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepTools.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Quantity_Color.hxx>
#include <STEPControl_Reader.hxx>
#include <Standard_Failure.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <WNT_Window.hxx>
#include <IGESControl_Reader.hxx>

#include <QFileInfo>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPixmap>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QShowEvent>

namespace occtdebug
{
namespace
{
struct GeometryObjectRef
{
    TopAbs_ShapeEnum type = TopAbs_SHAPE;
    int index = 0;
    QString normalizedId;
};

QString prefixForShapeType(TopAbs_ShapeEnum type)
{
    switch (type)
    {
    case TopAbs_VERTEX:
        return QStringLiteral("V");
    case TopAbs_EDGE:
        return QStringLiteral("E");
    case TopAbs_WIRE:
        return QStringLiteral("W");
    case TopAbs_FACE:
        return QStringLiteral("F");
    case TopAbs_SHELL:
        return QStringLiteral("SHELL");
    case TopAbs_SOLID:
        return QStringLiteral("SOLID");
    case TopAbs_COMPSOLID:
        return QStringLiteral("COMPSOLID");
    case TopAbs_COMPOUND:
        return QStringLiteral("COMPOUND");
    case TopAbs_SHAPE:
        return QStringLiteral("SHAPE");
    }
    return QStringLiteral("SHAPE");
}

QString nameForShapeType(TopAbs_ShapeEnum type)
{
    switch (type)
    {
    case TopAbs_VERTEX:
        return QStringLiteral("vertex");
    case TopAbs_EDGE:
        return QStringLiteral("edge");
    case TopAbs_WIRE:
        return QStringLiteral("wire");
    case TopAbs_FACE:
        return QStringLiteral("face");
    case TopAbs_SHELL:
        return QStringLiteral("shell");
    case TopAbs_SOLID:
        return QStringLiteral("solid");
    case TopAbs_COMPSOLID:
        return QStringLiteral("compsolid");
    case TopAbs_COMPOUND:
        return QStringLiteral("compound");
    case TopAbs_SHAPE:
        return QStringLiteral("shape");
    }
    return QStringLiteral("shape");
}

int subShapeCount(const TopoDS_Shape& shape, TopAbs_ShapeEnum type)
{
    if (shape.IsNull())
    {
        return 0;
    }
    int count = 0;
    for (TopExp_Explorer explorer(shape, type); explorer.More(); explorer.Next())
    {
        ++count;
    }
    return count;
}

bool parseGeometryObjectId(const QString& raw, GeometryObjectRef* out, QString* error)
{
    const QString value = raw.trimmed();
    if (value.isEmpty())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("geometry object id is empty");
        }
        return false;
    }

    const QRegularExpression pattern(QStringLiteral("^([A-Za-z]+)[_-]?(\\d+)$"));
    const QRegularExpressionMatch match = pattern.match(value);
    if (!match.hasMatch())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("unsupported geometry object id: %1").arg(value);
        }
        return false;
    }

    const QString prefix = match.captured(1).toLower();
    const int index = match.captured(2).toInt();
    if (index <= 0)
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("geometry object index must be positive: %1").arg(value);
        }
        return false;
    }

    TopAbs_ShapeEnum type = TopAbs_SHAPE;
    QString normalizedPrefix;
    if (prefix == QStringLiteral("v") || prefix == QStringLiteral("vertex"))
    {
        type = TopAbs_VERTEX;
    }
    else if (prefix == QStringLiteral("e") || prefix == QStringLiteral("edge"))
    {
        type = TopAbs_EDGE;
    }
    else if (prefix == QStringLiteral("w") || prefix == QStringLiteral("wire"))
    {
        type = TopAbs_WIRE;
    }
    else if (prefix == QStringLiteral("f") || prefix == QStringLiteral("face"))
    {
        type = TopAbs_FACE;
    }
    else if (prefix == QStringLiteral("shell"))
    {
        type = TopAbs_SHELL;
    }
    else if (prefix == QStringLiteral("solid"))
    {
        type = TopAbs_SOLID;
    }
    else if (prefix == QStringLiteral("compsolid"))
    {
        type = TopAbs_COMPSOLID;
    }
    else if (prefix == QStringLiteral("compound"))
    {
        type = TopAbs_COMPOUND;
    }
    else if (prefix == QStringLiteral("shape"))
    {
        type = TopAbs_SHAPE;
    }
    else
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("unsupported geometry object prefix: %1").arg(prefix);
        }
        return false;
    }

    if (out != nullptr)
    {
        out->type = type;
        out->index = index;
        normalizedPrefix = prefixForShapeType(type);
        out->normalizedId = QStringLiteral("%1%2").arg(normalizedPrefix).arg(index);
    }
    return true;
}

bool findSubShapeByIndex(const TopoDS_Shape& shape, TopAbs_ShapeEnum type, int index, TopoDS_Shape* out)
{
    if (shape.IsNull() || index <= 0)
    {
        return false;
    }

    if (type == TopAbs_SHAPE)
    {
        if (index == 1)
        {
            if (out != nullptr)
            {
                *out = shape;
            }
            return true;
        }
        return false;
    }

    int current = 0;
    for (TopExp_Explorer explorer(shape, type); explorer.More(); explorer.Next())
    {
        ++current;
        if (current == index)
        {
            if (out != nullptr)
            {
                *out = explorer.Current();
            }
            return true;
        }
    }
    return false;
}
} // namespace

OcctViewerWidget::OcctViewerWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::StrongFocus);
}

QPaintEngine* OcctViewerWidget::paintEngine() const
{
    return nullptr;
}

QSize OcctViewerWidget::minimumSizeHint() const
{
    return QSize(360, 260);
}

void OcctViewerWidget::fitAll()
{
    if (!m_view.IsNull())
    {
        m_view->FitAll(0.01, false);
        m_view->Redraw();
    }
}

void OcctViewerWidget::clearView()
{
    initializeViewer();
    if (!m_context.IsNull())
    {
        m_context->RemoveAll(false);
    }
    m_displayedShape.Nullify();
    m_highlightedShape.Nullify();
    m_currentShape.Nullify();
    m_highlightedObjectId.clear();
    if (!m_view.IsNull())
    {
        m_view->Redraw();
    }
}

void OcctViewerWidget::displayDemoShape()
{
    const TopoDS_Shape shape = BRepPrimAPI_MakeBox(120.0, 80.0, 60.0).Shape();
    displayShape(shape);
}

bool OcctViewerWidget::loadModelFile(const QString& filePath, QString* error)
{
    initializeViewer();

    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("model file does not exist: %1").arg(filePath);
        }
        return false;
    }

    const QString suffix = fileInfo.suffix().toLower();
    const bool isBrep = suffix == QStringLiteral("brep") || suffix == QStringLiteral("brp") || suffix == QStringLiteral("rle");
    const bool isStep = suffix == QStringLiteral("step") || suffix == QStringLiteral("stp");
    const bool isIges = suffix == QStringLiteral("iges") || suffix == QStringLiteral("igs");
    if (!isBrep && !isStep && !isIges)
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("unsupported model format: .%1").arg(suffix);
        }
        return false;
    }

    TopoDS_Shape shape;
    try
    {
        const QByteArray nativePath = fileInfo.absoluteFilePath().toLocal8Bit();
        if (isBrep)
        {
            BRep_Builder builder;
            if (!BRepTools::Read(shape, nativePath.constData(), builder) || shape.IsNull())
            {
                if (error != nullptr)
                {
                    *error = QStringLiteral("failed to read BREP model: %1").arg(filePath);
                }
                return false;
            }
        }
        else if (isStep)
        {
            STEPControl_Reader reader;
            const IFSelect_ReturnStatus status = reader.ReadFile(nativePath.constData());
            if (status != IFSelect_RetDone || reader.TransferRoots() <= 0)
            {
                if (error != nullptr)
                {
                    *error = QStringLiteral("failed to read STEP model: %1").arg(filePath);
                }
                return false;
            }
            shape = reader.OneShape();
        }
        else if (isIges)
        {
            IGESControl_Reader reader;
            const IFSelect_ReturnStatus status = reader.ReadFile(nativePath.constData());
            if (status != IFSelect_RetDone || reader.TransferRoots() <= 0)
            {
                if (error != nullptr)
                {
                    *error = QStringLiteral("failed to read IGES model: %1").arg(filePath);
                }
                return false;
            }
            shape = reader.OneShape();
        }

        if (shape.IsNull())
        {
            if (error != nullptr)
            {
                *error = QStringLiteral("OCCT returned an empty shape: %1").arg(filePath);
            }
            return false;
        }
    }
    catch (const Standard_Failure& failure)
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("OCCT failed to read BREP: %1").arg(QString::fromLocal8Bit(failure.GetMessageString()));
        }
        return false;
    }

    displayShape(shape);
    return true;
}

bool OcctViewerWidget::highlightGeometryObject(const QString& objectId, QString* error)
{
    initializeViewer();
    if (m_context.IsNull())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("OCCT viewer context is not initialized");
        }
        return false;
    }
    if (m_currentShape.IsNull())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("no geometry shape is loaded");
        }
        return false;
    }

    GeometryObjectRef ref;
    if (!parseGeometryObjectId(objectId, &ref, error))
    {
        return false;
    }

    TopoDS_Shape subShape;
    if (!findSubShapeByIndex(m_currentShape, ref.type, ref.index, &subShape) || subShape.IsNull())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("%1 was not found in the currently loaded shape").arg(ref.normalizedId);
        }
        clearHighlight();
        return false;
    }

    clearHighlight();
    Handle(AIS_Shape) highlight = new AIS_Shape(subShape);
    highlight->SetColor(Quantity_Color(1.0, 0.15, 0.08, Quantity_TOC_RGB));
    highlight->SetWidth(4.0);
    m_context->Display(highlight, false);
    m_highlightedShape = highlight;
    m_highlightedObjectId = ref.normalizedId;
    if (!m_view.IsNull())
    {
        m_view->Redraw();
    }
    return true;
}

void OcctViewerWidget::clearHighlight()
{
    if (!m_context.IsNull() && !m_highlightedShape.IsNull())
    {
        m_context->Remove(m_highlightedShape, false);
    }
    m_highlightedShape.Nullify();
    m_highlightedObjectId.clear();
    if (!m_view.IsNull())
    {
        m_view->Redraw();
    }
}

QString OcctViewerWidget::highlightedObjectId() const
{
    return m_highlightedObjectId;
}

QString OcctViewerWidget::topologySummary() const
{
    if (m_currentShape.IsNull())
    {
        return QStringLiteral("no shape loaded");
    }

    return QStringLiteral("topology: V=%1 E=%2 W=%3 F=%4 SHELL=%5 SOLID=%6 COMPSOLID=%7 COMPOUND=%8")
        .arg(subShapeCount(m_currentShape, TopAbs_VERTEX))
        .arg(subShapeCount(m_currentShape, TopAbs_EDGE))
        .arg(subShapeCount(m_currentShape, TopAbs_WIRE))
        .arg(subShapeCount(m_currentShape, TopAbs_FACE))
        .arg(subShapeCount(m_currentShape, TopAbs_SHELL))
        .arg(subShapeCount(m_currentShape, TopAbs_SOLID))
        .arg(subShapeCount(m_currentShape, TopAbs_COMPSOLID))
        .arg(subShapeCount(m_currentShape, TopAbs_COMPOUND));
}

QString OcctViewerWidget::topologySignatureForObject(const QString& objectId, QString* error) const
{
    return TopologySignature::stableIdForObject(m_currentShape, objectId, error);
}

QJsonObject OcctViewerWidget::topologySignatureJson(const QString& sourceLabel, const QString& selectedObjectId) const
{
    return TopologySignature::build(m_currentShape, sourceLabel, selectedObjectId);
}

bool OcctViewerWidget::saveScreenshot(const QString& filePath, QString* error)
{
    if (filePath.trimmed().isEmpty())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("screenshot path is empty");
        }
        return false;
    }

    initializeViewer();
    if (!m_view.IsNull())
    {
        m_view->Redraw();
    }

    const QPixmap pixmap = grab();
    if (pixmap.isNull())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("failed to capture viewer pixmap");
        }
        return false;
    }
    if (!pixmap.save(filePath, "PNG"))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("failed to save viewer screenshot: %1").arg(filePath);
        }
        return false;
    }
    return true;
}

void OcctViewerWidget::displayShape(const TopoDS_Shape& shape)
{
    initializeViewer();
    if (m_context.IsNull() || shape.IsNull())
    {
        return;
    }

    m_context->RemoveAll(false);
    Handle(AIS_Shape) aisShape = new AIS_Shape(shape);
    m_context->Display(aisShape, false);
    activateSubshapeSelection(aisShape);
    m_displayedShape = aisShape;
    m_highlightedShape.Nullify();
    m_currentShape = shape;
    m_highlightedObjectId.clear();
    fitAll();
}

void OcctViewerWidget::mousePressEvent(QMouseEvent* event)
{
    if (event == nullptr || event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }

    initializeViewer();
    if (m_context.IsNull() || m_view.IsNull() || m_currentShape.IsNull())
    {
        QWidget::mousePressEvent(event);
        return;
    }

    m_context->MoveTo(event->position().toPoint().x(), event->position().toPoint().y(), m_view, true);
    if (!m_context->HasDetected())
    {
        QWidget::mousePressEvent(event);
        return;
    }

    TopoDS_Shape pickedShape;
    if (m_context->HasDetectedShape())
    {
        pickedShape = m_context->DetectedShape();
    }
    if (pickedShape.IsNull())
    {
        QWidget::mousePressEvent(event);
        return;
    }

    const QString objectId = objectIdForShape(pickedShape);
    if (objectId.isEmpty())
    {
        QWidget::mousePressEvent(event);
        return;
    }

    QString highlightError;
    highlightGeometryObject(objectId, &highlightError);
    QString signatureError;
    const QString stableId = topologySignatureForObject(objectId, &signatureError);
    emit geometryObjectPicked(
        objectId,
        stableId.isEmpty()
            ? QStringLiteral("%1 selected by viewer pick; basis=TopExp index; signature_error=%2; %3")
                .arg(objectId, signatureError, topologySummary())
            : QStringLiteral("%1 selected by viewer pick; stable_id=%2; %3")
                .arg(objectId, stableId, topologySummary()));
}

void OcctViewerWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    initializeViewer();
    if (!m_view.IsNull())
    {
        m_view->Redraw();
    }
}

void OcctViewerWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (!m_view.IsNull())
    {
        m_view->MustBeResized();
    }
}

void OcctViewerWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    initializeViewer();
}

void OcctViewerWidget::initializeViewer()
{
    if (m_initialized)
    {
        return;
    }

    const Handle(Aspect_DisplayConnection) displayConnection = new Aspect_DisplayConnection();
    const Handle(OpenGl_GraphicDriver) graphicDriver = new OpenGl_GraphicDriver(displayConnection);
    m_viewer = new V3d_Viewer(graphicDriver);
    m_viewer->SetDefaultLights();
    m_viewer->SetLightOn();

    m_context = new AIS_InteractiveContext(m_viewer);
    m_view = m_viewer->CreateView();

    const Handle(WNT_Window) window = new WNT_Window(reinterpret_cast<Aspect_Handle>(winId()));
    m_view->SetWindow(window);
    if (!window->IsMapped())
    {
        window->Map();
    }

    m_view->SetBackgroundColor(Quantity_Color(0.02, 0.03, 0.05, Quantity_TOC_RGB));
    m_view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_Color(0.7, 0.8, 0.9, Quantity_TOC_RGB), 0.08, V3d_ZBUFFER);

    m_initialized = true;
    displayDemoShape();
}

QString OcctViewerWidget::objectIdForShape(const TopoDS_Shape& shape) const
{
    if (shape.IsNull() || m_currentShape.IsNull())
    {
        return QString();
    }
    if (shape.IsSame(m_currentShape))
    {
        return QStringLiteral("SHAPE1");
    }

    const TopAbs_ShapeEnum type = shape.ShapeType();
    if (type == TopAbs_SHAPE)
    {
        return QStringLiteral("SHAPE1");
    }

    int index = 0;
    for (TopExp_Explorer explorer(m_currentShape, type); explorer.More(); explorer.Next())
    {
        ++index;
        if (explorer.Current().IsSame(shape))
        {
            return QStringLiteral("%1%2").arg(prefixForShapeType(type)).arg(index);
        }
    }
    return QString();
}

void OcctViewerWidget::activateSubshapeSelection(const Handle(AIS_InteractiveObject)& object)
{
    if (m_context.IsNull() || object.IsNull())
    {
        return;
    }

    m_context->Activate(object, AIS_Shape::SelectionMode(TopAbs_VERTEX), false);
    m_context->Activate(object, AIS_Shape::SelectionMode(TopAbs_EDGE), false);
    m_context->Activate(object, AIS_Shape::SelectionMode(TopAbs_FACE), false);
    m_context->Activate(object, AIS_Shape::SelectionMode(TopAbs_SOLID), false);
    m_context->Activate(object, AIS_Shape::SelectionMode(TopAbs_COMPSOLID), false);
    m_context->Activate(object, AIS_Shape::SelectionMode(TopAbs_COMPOUND), false);
}
} // namespace occtdebug
