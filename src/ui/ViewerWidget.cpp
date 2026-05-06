#include "ui/ViewerWidget.h"

#include <cstring>

#include <algorithm>
#include <cmath>

#include <QLabel>
#include <QEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QWindow>

#if defined(_WIN32)

#include <AIS_DisplayMode.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_Handle.hxx>
#include <Graphic3d_ZLayerId.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Quantity_Color.hxx>
#include <Quantity_NameOfColor.hxx>
#include <V3d_TypeOfOrientation.hxx>
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
    bool initialized = false;
};

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
    // Avoid QSplitter + native child collapsing the GL surface to a thin strip.
    setMinimumHeight(64);
    m_deferredViewportTimer = new QTimer(this);
    m_deferredViewportTimer->setSingleShot(true);
    connect(m_deferredViewportTimer, &QTimer::timeout, this, [this]() {
        if (m_occt && m_occt->initialized && !m_occt->view.IsNull())
        {
            syncOcctViewport();
            redrawView();
        }
    });
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
    // Windows erases HWND with WM_ERASEBKGND before paint; without handling, the GL buffer goes
    // black after focus/click (especially after picking a sub-shape in AIS).
    //
    // Do not include <windows.h> here: OCCT/Qt headers can leave "MSG" incomplete or hide WM_*.
    // Qt passes a pointer to Win32 MSG; after HWND comes UINT message (Win32/Win64).
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
                syncOcctViewport();
                m_occt->view->Redraw();
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
    m_occt->driver = new OpenGl_GraphicDriver(m_occt->display, false);
    m_occt->viewer = new V3d_Viewer(m_occt->driver);
    m_occt->viewer->SetDefaultLights();
    m_occt->viewer->SetLightOn();
    m_occt->view = m_occt->viewer->CreateView();

    const Aspect_Handle h = reinterpret_cast<Aspect_Handle>(wid);
    // Must match V3d_View background; BLACK here leaves Win32 erase/unpainted areas black → flicker vs gray.
    m_occt->wntWindow = new WNT_Window(h, Quantity_NOC_GRAY45);
    m_occt->view->SetWindow(m_occt->wntWindow);
    if (!m_occt->wntWindow->IsMapped())
    {
        m_occt->wntWindow->Map();
    }

    m_occt->context = new AIS_InteractiveContext(m_occt->viewer);
    m_occt->view->SetBackgroundColor(Quantity_Color(Quantity_NOC_GRAY45));
    m_occt->view->SetProj(V3d_TypeOfOrientation_Zup_Front);
    m_occt->initialized = true;
    syncOcctViewport();
    m_occt->view->Redraw();
#endif
}

void ViewerWidget::redrawView()
{
#if defined(_WIN32)
    if (m_occt && m_occt->initialized && !m_occt->view.IsNull())
    {
        m_occt->view->Redraw();
    }
#endif
}

#if defined(_WIN32)
qreal ViewerWidget::effectiveDevicePixelRatio() const
{
    if (window() && window()->windowHandle())
    {
        return window()->windowHandle()->devicePixelRatio();
    }
    return devicePixelRatioF();
}

QPoint ViewerWidget::mapToOcctPixels(const QPointF& pos) const
{
    const qreal dpr = effectiveDevicePixelRatio();
    return QPoint(
        static_cast<int>(std::floor(pos.x() * dpr)),
        static_cast<int>(std::floor(pos.y() * dpr)));
}

void ViewerWidget::syncOcctViewport()
{
    if (!m_occt || !m_occt->initialized || m_occt->wntWindow.IsNull() || m_occt->view.IsNull())
    {
        return;
    }

    // Logical widget size × DPR must match the HWND client area. metric(Pdm*) can lag behind
    // splitter/layout updates; width()/height() track the GL drawable extent more reliably.
    const qreal dpr = effectiveDevicePixelRatio();
    const int w = std::max(1, width());
    const int h = std::max(1, height());
    const int pw = std::max(1, static_cast<int>(std::ceil(static_cast<qreal>(w) * dpr)));
    const int ph = std::max(1, static_cast<int>(std::ceil(static_cast<qreal>(h) * dpr)));
    m_occt->wntWindow->SetPos(0, 0, pw - 1, ph - 1);
    (void)m_occt->wntWindow->DoResize();
    m_occt->view->MustBeResized();
}

void ViewerWidget::scheduleDeferredViewportSync()
{
    if (m_deferredViewportTimer)
    {
        m_deferredViewportTimer->start(0);
    }
}
#endif

void ViewerWidget::deferViewportSync()
{
#if defined(_WIN32)
    ensureOcctInitialized();
    scheduleDeferredViewportSync();
#endif
}

void ViewerWidget::setRootShape(const TopoDS_Shape& root)
{
#if defined(_WIN32)
    ensureOcctInitialized();
    if (!m_occt || !m_occt->initialized || m_occt->context.IsNull())
    {
        return;
    }

    syncOcctViewport();

    if (!m_occt->highlightAis.IsNull())
    {
        m_occt->context->Remove(m_occt->highlightAis, Standard_False);
        m_occt->highlightAis.Nullify();
    }
    if (!m_occt->rootAis.IsNull())
    {
        m_occt->context->Remove(m_occt->rootAis, Standard_False);
        m_occt->rootAis.Nullify();
    }

    if (root.IsNull())
    {
        m_occt->view->Redraw();
        return;
    }

    m_occt->rootAis = new AIS_Shape(root);
    m_occt->context->Display(m_occt->rootAis, Standard_False);
    m_occt->context->SetDisplayMode(m_occt->rootAis, AIS_Shaded, Standard_True);
    m_occt->view->FitAll();
    m_occt->view->Redraw();
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

    syncOcctViewport();

    if (!m_occt->highlightAis.IsNull())
    {
        m_occt->context->Remove(m_occt->highlightAis, Standard_False);
        m_occt->highlightAis.Nullify();
    }

    if (shape.IsNull())
    {
        m_occt->context->UpdateCurrentViewer();
        m_occt->view->Redraw();
        scheduleDeferredViewportSync();
        return;
    }

    m_occt->highlightAis = new AIS_Shape(shape);
    m_occt->highlightAis->SetDisplayMode(AIS_WireFrame);
    m_occt->highlightAis->SetColor(Quantity_Color(Quantity_NOC_YELLOW));
    m_occt->highlightAis->SetWidth(2.5);
    m_occt->context->Display(m_occt->highlightAis, Standard_False);
    m_occt->context->SetDisplayMode(m_occt->highlightAis, AIS_WireFrame, Standard_False);
    m_occt->context->SetZLayer(m_occt->highlightAis, Graphic3d_ZLayerId_Top);
    m_occt->context->UpdateCurrentViewer();
    m_occt->view->Redraw();
    scheduleDeferredViewportSync();
#endif
}

void ViewerWidget::fitAll()
{
#if defined(_WIN32)
    if (!m_occt || !m_occt->initialized || m_occt->view.IsNull())
    {
        return;
    }
    syncOcctViewport();
    m_occt->view->FitAll();
    m_occt->view->Redraw();
#endif
}

void ViewerWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
#if defined(_WIN32)
    ensureOcctInitialized();
    if (m_occt && m_occt->initialized && !m_occt->view.IsNull())
    {
        syncOcctViewport();
        redrawView();
        scheduleDeferredViewportSync();
    }
#endif
}

void ViewerWidget::paintEvent(QPaintEvent* event)
{
#if defined(_WIN32)
    if (m_occt && m_occt->initialized && !m_occt->view.IsNull())
    {
        // Do not call V3d_View::Redraw() inside Qt paint (WM_PAINT): it re-enters GL and causes
        // black/gray flashing when combined with focus changes and WM_ERASEBKGND.
        scheduleDeferredViewportSync();
        event->accept();
        return;
    }
#endif
    QWidget::paintEvent(event);
}

void ViewerWidget::focusInEvent(QFocusEvent* event)
{
    QWidget::focusInEvent(event);
#if defined(_WIN32)
    syncOcctViewport();
    redrawView();
#endif
}

void ViewerWidget::focusOutEvent(QFocusEvent* event)
{
    QWidget::focusOutEvent(event);
#if defined(_WIN32)
    syncOcctViewport();
    redrawView();
#endif
}

void ViewerWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
#if defined(_WIN32)
    ensureOcctInitialized();
    syncOcctViewport();
    redrawView();
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
        redrawView();
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
            redrawView();
        }
        else if (m_panning && (event->buttons() & Qt::MiddleButton))
        {
            m_occt->view->Pan(p.x() - m_lastPos.x(), m_lastPos.y() - p.y(), 1.0, Standard_False);
            m_lastPos = p;
            redrawView();
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
        redrawView();
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
            const QPoint p = mapToOcctPixels(event->position());
            const int step = (dy > 0) ? -10 : 10;
            m_occt->view->Zoom(p.x(), p.y(), p.x(), p.y() + step);
            redrawView();
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

void ViewerWidget::changeEvent(QEvent* event)
{
#if defined(_WIN32)
    if (event->type() == QEvent::DevicePixelRatioChange)
    {
        syncOcctViewport();
        redrawView();
    }
#endif
    QWidget::changeEvent(event);
}
