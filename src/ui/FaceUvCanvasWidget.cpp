#include "ui/FaceUvCanvasWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace
{
constexpr int kMarginLeft = 52;
constexpr int kMarginRight = 12;
constexpr int kMarginTop = 14;
constexpr int kMarginBottom = 40;
constexpr double kZoomFactorPerNotch = 1.14;
constexpr double kMinSpan = 1e-14;
} // namespace

FaceUvCanvasWidget::FaceUvCanvasWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(200);
    setAutoFillBackground(true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void FaceUvCanvasWidget::setPolylines(const std::vector<FaceUvPolyline>& polys)
{
    m_polys = polys;
    recomputeDataBounds();
    fitViewToData();
    update();
}

void FaceUvCanvasWidget::recomputeDataBounds()
{
    m_hasData = false;
    m_du0 = 0.0;
    m_du1 = 1.0;
    m_dv0 = 0.0;
    m_dv1 = 1.0;
    bool any = false;
    double u0 = 0;
    double u1 = 0;
    double v0 = 0;
    double v1 = 0;
    for (const FaceUvPolyline& p : m_polys)
    {
        for (const auto& q : p.points)
        {
            if (!any)
            {
                u0 = u1 = q.first;
                v0 = v1 = q.second;
                any = true;
            }
            else
            {
                u0 = std::min(u0, q.first);
                u1 = std::max(u1, q.first);
                v0 = std::min(v0, q.second);
                v1 = std::max(v1, q.second);
            }
        }
    }
    if (!any)
    {
        return;
    }
    m_hasData = true;
    const double du = std::max(kMinSpan, u1 - u0);
    const double dv = std::max(kMinSpan, v1 - v0);
    const double m = 0.06 * std::max(du, dv);
    m_du0 = u0 - m;
    m_du1 = u1 + m;
    m_dv0 = v0 - m;
    m_dv1 = v1 + m;
}

void FaceUvCanvasWidget::fitViewToData()
{
    if (!m_hasData)
    {
        m_vu0 = 0.0;
        m_vu1 = 1.0;
        m_vv0 = 0.0;
        m_vv1 = 1.0;
        return;
    }
    m_vu0 = m_du0;
    m_vu1 = m_du1;
    m_vv0 = m_dv0;
    m_vv1 = m_dv1;
}

QRect FaceUvCanvasWidget::plotRect() const
{
    return rect().adjusted(kMarginLeft, kMarginTop, -kMarginRight, -kMarginBottom);
}

QPointF FaceUvCanvasWidget::widgetToUv(QPointF px) const
{
    const QRect pr = plotRect();
    if (pr.width() < 1 || pr.height() < 1)
    {
        return QPointF(0.0, 0.0);
    }
    const double ru = m_vu1 - m_vu0;
    const double rv = m_vv1 - m_vv0;
    const double u = m_vu0 + (px.x() - pr.left()) / static_cast<double>(pr.width()) * ru;
    const double v = m_vv0 + (pr.bottom() - px.y()) / static_cast<double>(pr.height()) * rv;
    return QPointF(u, v);
}

QPointF FaceUvCanvasWidget::uvToWidget(double u, double v) const
{
    const QRect pr = plotRect();
    const double ru = m_vu1 - m_vu0;
    const double rv = m_vv1 - m_vv0;
    const double x = pr.left() + (u - m_vu0) / ru * pr.width();
    const double y = pr.bottom() - (v - m_vv0) / rv * pr.height();
    return QPointF(x, y);
}

void FaceUvCanvasWidget::drawAxesAndGrid(QPainter& g, const QRect& pr) const
{
    if (pr.width() < 4 || pr.height() < 4)
    {
        return;
    }
    const QPointF origin = uvToWidget(0.0, 0.0);
    g.setPen(QPen(QColor(140, 140, 150), 1));
    // u axis (horizontal baseline of plot)
    g.drawLine(pr.left(), pr.bottom(), pr.right(), pr.bottom());
    // v axis (vertical left of plot)
    g.drawLine(pr.left(), pr.top(), pr.left(), pr.bottom());

    // Arrows: u increases to the right
    {
        const QPoint a(pr.right() - 8, pr.bottom());
        g.drawLine(pr.right() - 18, pr.bottom() - 4, a.x(), a.y());
        g.drawLine(pr.right() - 18, pr.bottom() + 4, a.x(), a.y());
    }
    {
        const QPoint a(pr.left(), pr.top() + 8);
        g.drawLine(pr.left() - 4, pr.top() + 18, a.x(), a.y());
        g.drawLine(pr.left() + 4, pr.top() + 18, a.x(), a.y());
    }

    g.setPen(QColor(50, 50, 60));
    g.drawText(pr.right() - 22, pr.bottom() + 28, QStringLiteral("u"));
    g.drawText(pr.left() - 40, pr.top() + 14, QStringLiteral("v"));

    // Light grid
    g.setPen(QPen(QColor(230, 230, 235), 1, Qt::DotLine));
    const int n = 6;
    for (int i = 1; i < n; ++i)
    {
        const int x = pr.left() + (pr.width() * i) / n;
        g.drawLine(x, pr.top(), x, pr.bottom());
        const int y = pr.top() + (pr.height() * i) / n;
        g.drawLine(pr.left(), y, pr.right(), y);
    }

    // Origin marker if visible
    if (origin.x() >= pr.left() && origin.x() <= pr.right() && origin.y() >= pr.top()
        && origin.y() <= pr.bottom())
    {
        g.setPen(QPen(QColor(200, 80, 80), 1));
        g.drawEllipse(origin, 3, 3);
        g.drawText(origin + QPointF(6, -4), QStringLiteral("O"));
    }
}

void FaceUvCanvasWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    QPainter g(this);
    g.fillRect(rect(), QColor(250, 250, 252));
    g.setPen(QPen(QColor(60, 60, 70), 1));
    g.drawRect(rect().adjusted(0, 0, -1, -1));

    const QRect pr = plotRect();
    drawAxesAndGrid(g, pr);

    if (!m_hasData || m_polys.empty())
    {
        g.setPen(QColor(100, 100, 110));
        g.drawText(pr.center() - QPoint(120, 0), QStringLiteral("No UV data"));
        return;
    }

    g.setRenderHint(QPainter::Antialiasing, true);
    for (const FaceUvPolyline& pl : m_polys)
    {
        if (pl.points.size() < 2)
        {
            continue;
        }
        const QColor stroke = pl.outerWire ? QColor(0, 90, 200) : QColor(200, 90, 0);
        g.setPen(QPen(stroke, pl.outerWire ? 2.2 : 1.4));
        QPainterPath path;
        path.moveTo(uvToWidget(pl.points[0].first, pl.points[0].second));
        for (size_t i = 1; i < pl.points.size(); ++i)
        {
            path.lineTo(uvToWidget(pl.points[i].first, pl.points[i].second));
        }
        g.drawPath(path);

        g.setPen(QPen(stroke.lighter(120), 1));
        const size_t step = std::max<size_t>(1, pl.points.size() / 16);
        for (size_t i = step; i + step < pl.points.size(); i += step)
        {
            const QPointF pa = uvToWidget(pl.points[i - step / 2].first, pl.points[i - step / 2].second);
            const QPointF pb = uvToWidget(pl.points[i + step / 2].first, pl.points[i + step / 2].second);
            const QPointF dir = pb - pa;
            const double len = std::hypot(dir.x(), dir.y());
            if (len < 1e-3)
            {
                continue;
            }
            const QPointF n(-dir.y() / len, dir.x() / len);
            const QPointF mid = (pa + pb) * 0.5;
            g.drawLine(mid, mid + n * 5.0);
        }
    }

    g.setPen(QColor(80, 80, 90));
    g.drawText(
        kMarginLeft,
        height() - 6,
        QStringLiteral("Wheel: zoom at cursor · Middle drag: pan · Double-click: fit all"));
}

void FaceUvCanvasWidget::wheelEvent(QWheelEvent* event)
{
    if (!m_hasData)
    {
        event->ignore();
        return;
    }
    const int dy = event->angleDelta().y();
    if (dy == 0)
    {
        return;
    }
    const QPointF pivot = widgetToUv(event->position());
    const double z = std::pow(kZoomFactorPerNotch, static_cast<double>(dy) / 120.0);
    double u0 = m_vu0;
    double u1 = m_vu1;
    double v0 = m_vv0;
    double v1 = m_vv1;
    u0 = pivot.x() - (pivot.x() - u0) / z;
    u1 = pivot.x() + (u1 - pivot.x()) / z;
    v0 = pivot.y() - (pivot.y() - v0) / z;
    v1 = pivot.y() + (v1 - pivot.y()) / z;
    const double ru = u1 - u0;
    const double rv = v1 - v0;
    if (ru < kMinSpan || rv < kMinSpan)
    {
        return;
    }
    m_vu0 = u0;
    m_vu1 = u1;
    m_vv0 = v0;
    m_vv1 = v1;
    update();
    event->accept();
}

void FaceUvCanvasWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton && m_hasData)
    {
        m_panning = true;
        m_lastPan = event->position().toPoint();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void FaceUvCanvasWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_panning && (event->buttons() & Qt::MiddleButton) && m_hasData)
    {
        const QPoint p = event->position().toPoint();
        const QRect pr = plotRect();
        const double ru = m_vu1 - m_vu0;
        const double rv = m_vv1 - m_vv0;
        if (pr.width() > 0 && pr.height() > 0)
        {
            const double du = -static_cast<double>(p.x() - m_lastPan.x()) / pr.width() * ru;
            const double dv = static_cast<double>(p.y() - m_lastPan.y()) / pr.height() * rv;
            m_vu0 += du;
            m_vu1 += du;
            m_vv0 += dv;
            m_vv1 += dv;
            update();
        }
        m_lastPan = p;
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void FaceUvCanvasWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton)
    {
        m_panning = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void FaceUvCanvasWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (m_hasData)
    {
        fitViewToData();
        update();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}
