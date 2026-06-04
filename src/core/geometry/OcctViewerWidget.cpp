#include "core/geometry/OcctViewerWidget.h"

#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Quantity_Color.hxx>
#include <WNT_Window.hxx>

#include <QPaintEvent>
#include <QResizeEvent>
#include <QShowEvent>

namespace occtdebug
{
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

void OcctViewerWidget::displayDemoShape()
{
    if (m_context.IsNull())
    {
        return;
    }

    m_context->RemoveAll(false);
    const TopoDS_Shape shape = BRepPrimAPI_MakeBox(120.0, 80.0, 60.0).Shape();
    Handle(AIS_Shape) aisShape = new AIS_Shape(shape);
    m_context->Display(aisShape, false);
    fitAll();
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
} // namespace occtdebug
