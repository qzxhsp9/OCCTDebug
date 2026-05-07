#include "ui/ViewerWidget.h"

#include <cstring>

#include <algorithm>
#include <cmath>

#include <QLabel>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QVBoxLayout>
#include <QWheelEvent>

#if defined(_WIN32)

#include "occt/BBoxWire.h"

#include <AIS_DisplayMode.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_Handle.hxx>
#include <Aspect_TypeOfTriedronPosition.hxx>
#include <Graphic3d_ZLayerId.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Quantity_Color.hxx>
#include <Quantity_NameOfColor.hxx>
#include <V3d_TypeOfOrientation.hxx>
#include <V3d_TypeOfVisualization.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <WNT_Window.hxx>

struct ViewerOcctData
{
    Handle(Aspect_DisplayConnection) display;
    Handle(OpenGl_GraphicDriver) driver;
    Handle(V3d_Viewer) viewer;
    Handle(V3d_View) view;
    Handle(WNT_Window) wntWindow;
    Handle(AIS_InteractiveContext) context;
    Handle(AIS_Shape) rootAis;
    Handle(AIS_Shape) highlightAis;
    Handle(AIS_Shape) bboxAis;
    bool initialized = false;
};

namespace
{
void physicalWidgetPixels(const QWidget* w, int& outW, int& outH)
{
    qreal dpr = w->devicePixelRatioF() > 0.0 ? w->devicePixelRatioF() : 1.0;
    if (const QWidget* top = w->window())
    {
        const qreal topDpr = top->devicePixelRatioF() > 0.0 ? top->devicePixelRatioF() : 1.0;
        if (topDpr > dpr)
        {
            dpr = topDpr;
        }
    }
    const int lw = std::max(1, w->width());
    const int lh = std::max(1, w->height());
    outW = std::max(1, static_cast<int>(std::ceil(static_cast<qreal>(lw) * dpr)));
    outH = std::max(1, static_cast<int>(std::ceil(static_cast<qreal>(lh) * dpr)));
}
} // namespace

#endif

ViewerWidget::ViewerWidget(QWidget* parent)
    : QWidget(parent)
{
#if defined(_WIN32)
    setAttribute(Qt::WA_NativeWindow, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumHeight(64);
#else
    auto* layout = new QVBoxLayout(this);
    m_stubLabel = new QLabel(
        tr("3D viewer uses AIS/V3d on Windows. Build on Windows with OpenGL-enabled OCCT for full support."),
        this);
    m_stubLabel->setWordWrap(true);
    layout->addWidget(m_stubLabel);
#endif
}

ViewerWidget::~ViewerWidget()
{
#if defined(_WIN32)
    if (m_occt && !m_occt->context.IsNull())
    {
        m_occt->context->RemoveAll(Standard_False);
    }
    m_occt.reset();
#endif
}

#if defined(_WIN32)
bool ViewerWidget::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
    if (eventType == "windows_generic_MSG" && message != nullptr)
    {
        constexpr unsigned int kWmEraseBkgnd = 0x0014u;
        unsigned int winMsg = 0;
        const auto* bytes = static_cast<const unsigned char*>(message);
        std::memcpy(&winMsg, bytes + sizeof(void*), sizeof(unsigned int));
        if (winMsg == kWmEraseBkgnd)
        {
            if (m_occt && m_occt->initialized && !m_occt->view.IsNull())
            {
                flushView();
                if (result)
                {
                    *result = 1;
                }
                return true;
            }
        }
    }
    return QWidget::nativeEvent(eventType, message, result);
}
#endif

void ViewerWidget::ensureOcctInitialized()
{
#if defined(_WIN32)
    if (m_occt && m_occt->initialized)
    {
        return;
    }
    if (width() < 8 || height() < 8)
    {
        return;
    }

    createWinId();
    const WId wid = effectiveWinId();
    if (!wid)
    {
        return;
    }

    if (!m_occt)
    {
        m_occt = std::make_unique<ViewerOcctData>();
    }

    m_occt->display = new Aspect_DisplayConnection();
    m_occt->driver = new OpenGl_GraphicDriver(m_occt->display);
    m_occt->viewer = new V3d_Viewer(m_occt->driver);
    m_occt->viewer->SetDefaultLights();
    m_occt->viewer->SetLightOn();
    m_occt->view = m_occt->viewer->CreateView();

    const Aspect_Handle h = reinterpret_cast<Aspect_Handle>(wid);
    m_occt->wntWindow = new WNT_Window(h, Quantity_NOC_GRAY45);
    m_occt->view->SetWindow(m_occt->wntWindow);
    if (!m_occt->wntWindow->IsMapped())
    {
        m_occt->wntWindow->Map();
    }

    m_occt->context = new AIS_InteractiveContext(m_occt->viewer);
    m_occt->view->SetBackgroundColor(Quantity_Color(Quantity_NOC_GRAY45));
    m_occt->view->SetProj(V3d_TypeOfOrientation_Zup_Front);
    m_occt->view->TriedronDisplay(
        Aspect_TOTP_LEFT_LOWER,
        Quantity_Color(Quantity_NOC_WHITE),
        0.055,
        V3d_WIREFRAME);

    m_occt->initialized = true;
    m_occt->context->UpdateCurrentViewer();
    flushView();
#endif
}

#if defined(_WIN32)
void ViewerWidget::presentOnly()
{
    if (!m_occt || !m_occt->initialized || m_occt->view.IsNull())
    {
        return;
    }
    m_occt->view->RedrawImmediate();
}

void ViewerWidget::flushView()
{
    if (!m_occt || !m_occt->initialized || m_occt->view.IsNull())
    {
        return;
    }
    syncOcctViewport();
    m_occt->view->Redraw();
}

void ViewerWidget::syncOcctViewport()
{
    if (!m_occt || !m_occt->initialized || m_occt->wntWindow.IsNull() || m_occt->view.IsNull())
    {
        return;
    }

    int pw = 1;
    int ph = 1;
    physicalWidgetPixels(this, pw, ph);

    m_occt->wntWindow->SetPos(0, 0, pw - 1, ph - 1);
    (void)m_occt->wntWindow->DoResize();
    m_occt->view->MustBeResized();
}

QPoint ViewerWidget::mapToOcctPixels(const QPointF& pos) const
{
    int pw = 1;
    int ph = 1;
    physicalWidgetPixels(this, pw, ph);
    const int lw = std::max(1, width());
    const int lh = std::max(1, height());
    const int x = static_cast<int>(std::floor(pos.x() * static_cast<qreal>(pw) / static_cast<qreal>(lw)));
    const int y = static_cast<int>(std::floor(pos.y() * static_cast<qreal>(ph) / static_cast<qreal>(lh)));
    return QPoint(x, y);
}

void ViewerWidget::updateBboxAis()
{
    if (!m_occt || !m_occt->initialized || m_occt->context.IsNull())
    {
        return;
    }

    if (!m_occt->bboxAis.IsNull())
    {
        m_occt->context->Remove(m_occt->bboxAis, Standard_False);
        m_occt->bboxAis.Nullify();
    }

    if (!m_showBoundingBox || m_lastHighlight.IsNull())
    {
        m_occt->context->UpdateCurrentViewer();
        return;
    }

    const TopoDS_Shape wireShape = OcctBuildBoundingBoxWire(m_lastHighlight);
    if (wireShape.IsNull())
    {
        m_occt->context->UpdateCurrentViewer();
        return;
    }

    m_occt->bboxAis = new AIS_Shape(wireShape);
    m_occt->bboxAis->SetDisplayMode(AIS_WireFrame);
    m_occt->bboxAis->SetColor(Quantity_Color(Quantity_NOC_CYAN));
    m_occt->bboxAis->SetWidth(1.2);
    m_occt->context->Display(m_occt->bboxAis, Standard_False);
    m_occt->context->SetDisplayMode(m_occt->bboxAis, AIS_WireFrame, Standard_False);
    m_occt->context->SetZLayer(m_occt->bboxAis, Graphic3d_ZLayerId_Topmost);
    m_occt->context->UpdateCurrentViewer();
}
#endif

void ViewerWidget::deferViewportSync()
{
#if defined(_WIN32)
    ensureOcctInitialized();
    flushView();
#endif
}

void ViewerWidget::refreshPresentation()
{
#if defined(_WIN32)
    ensureOcctInitialized();
    if (!m_occt || !m_occt->initialized || m_occt->context.IsNull())
    {
        return;
    }
    m_occt->context->UpdateCurrentViewer();
    flushView();
#endif
}

void ViewerWidget::setShowBoundingBox(bool on)
{
    if (m_showBoundingBox == on)
    {
        return;
    }
    m_showBoundingBox = on;
#if defined(_WIN32)
    ensureOcctInitialized();
    if (m_occt && m_occt->initialized)
    {
        updateBboxAis();
        flushView();
    }
#endif
}

bool ViewerWidget::showBoundingBox() const
{
    return m_showBoundingBox;
}

void ViewerWidget::setRootShape(const TopoDS_Shape& root)
{
#if defined(_WIN32)
    ensureOcctInitialized();
    if (!m_occt || !m_occt->initialized || m_occt->context.IsNull())
    {
        return;
    }

    if (!m_occt->highlightAis.IsNull())
    {
        m_occt->context->Remove(m_occt->highlightAis, Standard_False);
        m_occt->highlightAis.Nullify();
    }
    if (!m_occt->bboxAis.IsNull())
    {
        m_occt->context->Remove(m_occt->bboxAis, Standard_False);
        m_occt->bboxAis.Nullify();
    }
    m_lastHighlight.Nullify();
    if (!m_occt->rootAis.IsNull())
    {
        m_occt->context->Remove(m_occt->rootAis, Standard_False);
        m_occt->rootAis.Nullify();
    }

    if (root.IsNull())
    {
        m_occt->context->UpdateCurrentViewer();
        flushView();
        return;
    }

    m_occt->rootAis = new AIS_Shape(root);
    m_occt->context->Display(m_occt->rootAis, Standard_False);
    m_occt->context->SetDisplayMode(m_occt->rootAis, AIS_Shaded, Standard_True);
    m_occt->context->UpdateCurrentViewer();
    syncOcctViewport();
    m_occt->view->FitAll();
    flushView();
#endif
}

void ViewerWidget::setHighlightShape(const TopoDS_Shape& shape)
{
#if defined(_WIN32)
    ensureOcctInitialized();
    if (!m_occt || !m_occt->initialized || m_occt->context.IsNull())
    {
        return;
    }

    if (!m_occt->highlightAis.IsNull())
    {
        m_occt->context->Remove(m_occt->highlightAis, Standard_False);
        m_occt->highlightAis.Nullify();
    }

    m_lastHighlight = shape;

    if (shape.IsNull())
    {
        updateBboxAis();
        m_occt->context->UpdateCurrentViewer();
        flushView();
        return;
    }

    m_occt->highlightAis = new AIS_Shape(shape);
    m_occt->highlightAis->SetDisplayMode(AIS_WireFrame);
    m_occt->highlightAis->SetColor(Quantity_Color(Quantity_NOC_YELLOW));
    m_occt->highlightAis->SetWidth(2.5);
    m_occt->context->Display(m_occt->highlightAis, Standard_False);
    m_occt->context->SetDisplayMode(m_occt->highlightAis, AIS_WireFrame, Standard_False);
    m_occt->context->SetZLayer(m_occt->highlightAis, Graphic3d_ZLayerId_Top);
    updateBboxAis();
    m_occt->context->UpdateCurrentViewer();
    flushView();
#endif
}

void ViewerWidget::fitAll()
{
#if defined(_WIN32)
    if (!m_occt || !m_occt->initialized || m_occt->view.IsNull())
    {
        return;
    }
    m_occt->view->FitAll();
    m_occt->context->UpdateCurrentViewer();
    flushView();
#endif
}

void ViewerWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
#if defined(_WIN32)
    ensureOcctInitialized();
    flushView();
#endif
}

void ViewerWidget::paintEvent(QPaintEvent* event)
{
#if defined(_WIN32)
    if (m_occt && m_occt->initialized && !m_occt->view.IsNull())
    {
        if (!m_inPaintFlush)
        {
            m_inPaintFlush = true;
            presentOnly();
            m_inPaintFlush = false;
        }
        event->accept();
        return;
    }
#endif
    QWidget::paintEvent(event);
}

void ViewerWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
#if defined(_WIN32)
    refreshPresentation();
#endif
}

void ViewerWidget::focusInEvent(QFocusEvent* event)
{
    QWidget::focusInEvent(event);
#if defined(_WIN32)
    refreshPresentation();
#endif
}

void ViewerWidget::focusOutEvent(QFocusEvent* event)
{
    QWidget::focusOutEvent(event);
#if defined(_WIN32)
    if (m_occt && m_occt->initialized && !m_occt->view.IsNull())
    {
        update();
    }
#endif
}

void ViewerWidget::mousePressEvent(QMouseEvent* event)
{
#if defined(_WIN32)
    if (m_occt && m_occt->initialized && !m_occt->view.IsNull())
    {
        const QPoint p = mapToOcctPixels(event->position());
        if (event->button() == Qt::LeftButton)
        {
            m_rotating = true;
            m_occt->view->StartRotation(p.x(), p.y(), 0.4);
        }
        else if (event->button() == Qt::MiddleButton)
        {
            m_panning = true;
            m_occt->view->Pan(0, 0, 1.0, Standard_True);
        }
        m_lastPos = p;
        flushView();
        return;
    }
#endif
    QWidget::mousePressEvent(event);
}

void ViewerWidget::mouseMoveEvent(QMouseEvent* event)
{
#if defined(_WIN32)
    if (m_occt && m_occt->initialized && !m_occt->view.IsNull())
    {
        const QPoint p = mapToOcctPixels(event->position());
        if (m_rotating && (event->buttons() & Qt::LeftButton))
        {
            m_occt->view->Rotation(p.x(), p.y());
            flushView();
        }
        else if (m_panning && (event->buttons() & Qt::MiddleButton))
        {
            m_occt->view->Pan(p.x() - m_lastPos.x(), m_lastPos.y() - p.y(), 1.0, Standard_False);
            m_lastPos = p;
            flushView();
        }
        return;
    }
#endif
    QWidget::mouseMoveEvent(event);
}

void ViewerWidget::mouseReleaseEvent(QMouseEvent* event)
{
#if defined(_WIN32)
    if (event->button() == Qt::LeftButton)
    {
        m_rotating = false;
    }
    if (event->button() == Qt::MiddleButton)
    {
        m_panning = false;
    }
    if (m_occt && m_occt->initialized)
    {
        flushView();
    }
#endif
    QWidget::mouseReleaseEvent(event);
}

void ViewerWidget::wheelEvent(QWheelEvent* event)
{
#if defined(_WIN32)
    if (m_occt && m_occt->initialized && !m_occt->view.IsNull())
    {
        const int dy = event->angleDelta().y();
        if (dy != 0)
        {
            const QPoint c = mapToOcctPixels(event->position());
            const qreal notches = static_cast<qreal>(dy) / 120.0;
            const int delta = static_cast<int>(std::lround(notches * 42.0));
            m_occt->view->StartZoomAtPoint(c.x(), c.y());
            m_occt->view->ZoomAtPoint(c.x(), c.y(), c.x(), c.y() - delta);
            flushView();
        }
        event->accept();
        return;
    }
#endif
    QWidget::wheelEvent(event);
}

void ViewerWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
#if defined(_WIN32)
    if (m_occt && m_occt->initialized && !m_occt->view.IsNull())
    {
        fitAll();
        event->accept();
        return;
    }
#endif
    QWidget::mouseDoubleClickEvent(event);
}
