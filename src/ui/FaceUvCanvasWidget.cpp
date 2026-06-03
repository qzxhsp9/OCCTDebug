#include "ui/FaceUvCanvasWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPolygonF>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace
{
constexpr int kMarginLeft = 34;
constexpr int kMarginRight = 6;
constexpr int kMarginTop = 6;
constexpr int kMarginBottom = 30;
constexpr double kZoomFactorPerNotch = 1.14;
constexpr double kMinSpan = 1e-14;

double niceStep(double span, int targetTicks)
{
    const double rough = std::max(kMinSpan, span / std::max(1, targetTicks));
    const double power = std::pow(10.0, std::floor(std::log10(rough)));
    const double normalized = rough / power;
    if (normalized <= 1.0)
    {
        return power;
    }
    if (normalized <= 2.0)
    {
        return 2.0 * power;
    }
    if (normalized <= 5.0)
    {
        return 5.0 * power;
    }
    return 10.0 * power;
}

QString coordLabel(double value)
{
    return QString::number(value, 'g', 5);
}

QColor ringColor(bool outerWire)
{
    return outerWire ? QColor(0, 92, 190) : QColor(210, 95, 0);
}

void drawDirectionArrow(QPainter& g, const QPointF& a, const QPointF& b, const QColor& color)
{
    const QPointF dir = b - a;
    const double len = std::hypot(dir.x(), dir.y());
    if (len < 1e-3)
    {
        return;
    }
    const QPointF u = dir / len;
    const QPointF n(-u.y(), u.x());
    const QPointF tip = a + dir * 0.55;
    const double size = 13.0;
    QPolygonF arrow;
    arrow << tip << (tip - u * size + n * (size * 0.45)) << (tip - u * size - n * (size * 0.45));
    g.setPen(Qt::NoPen);
    g.setBrush(color);
    g.drawPolygon(arrow);
}
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
    const double ru = m_vu1 - m_vu0;
    const double rv = m_vv1 - m_vv0;
    if (ru <= 0.0 || rv <= 0.0)
    {
        return;
    }

    g.setPen(QPen(QColor(190, 190, 198), 1));
    g.drawRect(pr.adjusted(0, 0, -1, -1));

    const double uStep = niceStep(ru, 7);
    const double vStep = niceStep(rv, 5);
    const double uStart = std::ceil(m_vu0 / uStep) * uStep;
    const double vStart = std::ceil(m_vv0 / vStep) * vStep;

    g.setPen(QPen(QColor(230, 230, 235), 1, Qt::DotLine));
    for (double u = uStart; u <= m_vu1 + uStep * 0.25; u += uStep)
    {
        const int x = static_cast<int>(std::lround(uvToWidget(u, m_vv0).x()));
        if (x < pr.left() || x > pr.right())
        {
            continue;
        }
        g.drawLine(x, pr.top(), x, pr.bottom());
        g.setPen(QColor(80, 80, 90));
        g.drawText(x - 22, pr.bottom() + 10, 44, 16, Qt::AlignHCenter | Qt::AlignTop, coordLabel(u));
        g.setPen(QPen(QColor(230, 230, 235), 1, Qt::DotLine));
    }
    for (double v = vStart; v <= m_vv1 + vStep * 0.25; v += vStep)
    {
        const int y = static_cast<int>(std::lround(uvToWidget(m_vu0, v).y()));
        if (y < pr.top() || y > pr.bottom())
        {
            continue;
        }
        g.drawLine(pr.left(), y, pr.right(), y);
        g.setPen(QColor(80, 80, 90));
        g.drawText(2, y - 8, kMarginLeft - 8, 16, Qt::AlignRight | Qt::AlignVCenter, coordLabel(v));
        g.setPen(QPen(QColor(230, 230, 235), 1, Qt::DotLine));
    }

    const bool hasUAxis = m_vv0 <= 0.0 && m_vv1 >= 0.0;
    const bool hasVAxis = m_vu0 <= 0.0 && m_vu1 >= 0.0;
    const int uAxisY =
        hasUAxis ? static_cast<int>(std::lround(uvToWidget(m_vu0, 0.0).y())) : pr.bottom();
    const int vAxisX =
        hasVAxis ? static_cast<int>(std::lround(uvToWidget(0.0, m_vv0).x())) : pr.left();

    g.setPen(QPen(QColor(140, 140, 150), 1));
    if (hasUAxis)
    {
        g.drawLine(pr.left(), uAxisY, pr.right(), uAxisY);
    }
    if (hasVAxis)
    {
        g.drawLine(vAxisX, pr.top(), vAxisX, pr.bottom());
    }

    // Arrows show positive data directions; axis positions come from current UV coordinates.
    {
        const QPoint a(pr.right() - 8, uAxisY);
        g.drawLine(pr.right() - 18, uAxisY - 4, a.x(), a.y());
        g.drawLine(pr.right() - 18, uAxisY + 4, a.x(), a.y());
    }
    {
        const QPoint a(vAxisX, pr.top() + 8);
        g.drawLine(vAxisX - 4, pr.top() + 18, a.x(), a.y());
        g.drawLine(vAxisX + 4, pr.top() + 18, a.x(), a.y());
    }

    g.setPen(QColor(50, 50, 60));
    g.drawText(pr.right() - 22, std::min(pr.bottom() + 22, uAxisY + 22), QStringLiteral("u"));
    g.drawText(std::max(4, vAxisX - 28), pr.top() + 14, QStringLiteral("v"));

    // Origin marker if visible
    if (hasUAxis && hasVAxis)
    {
        const QPointF origin = uvToWidget(0.0, 0.0);
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

        std::vector<size_t> breaks = pl.edgeBreaks;
        if (breaks.size() < 2 || breaks.front() != 0 || breaks.back() != pl.points.size())
        {
            breaks = {0, pl.points.size()};
        }

        for (size_t edgeIdx = 0; edgeIdx + 1 < breaks.size(); ++edgeIdx)
        {
            const size_t first = breaks[edgeIdx];
            const size_t last = breaks[edgeIdx + 1];
            if (last <= first + 1 || last > pl.points.size())
            {
                continue;
            }

            const QColor stroke = ringColor(pl.outerWire);
            QPainterPath path;
            path.moveTo(uvToWidget(pl.points[first].first, pl.points[first].second));
            for (size_t i = first + 1; i < last; ++i)
            {
                path.lineTo(uvToWidget(pl.points[i].first, pl.points[i].second));
            }
            g.setPen(QPen(stroke, pl.outerWire ? 2.8 : 2.2));
            g.setBrush(Qt::NoBrush);
            g.drawPath(path);

            const size_t mid = first + (last - first) / 2;
            if (mid > first)
            {
                const QPointF pa = uvToWidget(pl.points[mid - 1].first, pl.points[mid - 1].second);
                const QPointF pb = uvToWidget(pl.points[mid].first, pl.points[mid].second);
                drawDirectionArrow(g, pa, pb, stroke.darker(120));
            }

            const QPointF vertex = uvToWidget(pl.points[first].first, pl.points[first].second);
            g.setPen(QPen(stroke.darker(150), 1.3));
            g.setBrush(QColor(255, 255, 255));
            g.drawEllipse(vertex, 4.0, 4.0);
            if (edgeIdx == 0)
            {
                g.setBrush(stroke);
                g.drawEllipse(vertex, 2.2, 2.2);
            }
        }

        const QPointF lastVertex = uvToWidget(pl.points.back().first, pl.points.back().second);
        g.setPen(QPen(QColor(55, 55, 65), 1.2));
        g.setBrush(QColor(255, 255, 255));
        g.drawRect(QRectF(lastVertex.x() - 3.5, lastVertex.y() - 3.5, 7.0, 7.0));
    }

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
