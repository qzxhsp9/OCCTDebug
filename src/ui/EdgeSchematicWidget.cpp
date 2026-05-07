#include "ui/EdgeSchematicWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace
{
constexpr int kMarginLeft = 48;
constexpr int kMarginRight = 14;
constexpr int kMarginTop = 12;
constexpr int kMarginBottom = 38;
constexpr double kZoomFactorPerNotch = 1.14;
constexpr double kMinSpan = 1e-6;

double tolRadiusPx(double tol)
{
    return std::clamp(-std::log10(std::max(tol, 1e-12)) * 3.0, 5.0, 38.0);
}
} // namespace

EdgeSchematicWidget::EdgeSchematicWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(160);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void EdgeSchematicWidget::setTolerances(double vertexTol0, double vertexTol1, double edgeTol)
{
    m_v0 = vertexTol0;
    m_v1 = vertexTol1;
    m_edge = edgeTol;
    resetTView();
    update();
}

void EdgeSchematicWidget::resetTView()
{
    m_t0 = 0.0;
    m_t1 = 1.0;
}

QRect EdgeSchematicWidget::plotRect() const
{
    return rect().adjusted(kMarginLeft, kMarginTop, -kMarginRight, -kMarginBottom);
}

double EdgeSchematicWidget::xToT(double px) const
{
    const QRect pr = plotRect();
    const double span = m_t1 - m_t0;
    if (pr.width() < 1)
    {
        return m_t0;
    }
    return m_t0 + (px - pr.left()) / static_cast<double>(pr.width()) * span;
}

double EdgeSchematicWidget::tToX(double t) const
{
    const QRect pr = plotRect();
    const double span = m_t1 - m_t0;
    return pr.left() + (t - m_t0) / span * pr.width();
}

void EdgeSchematicWidget::drawAxes(QPainter& p, const QRect& pr, int yMid) const
{
    p.setPen(QPen(QColor(130, 130, 140), 1));
    p.drawLine(pr.left(), pr.bottom(), pr.right(), pr.bottom());
    p.drawLine(pr.left(), pr.top(), pr.left(), pr.bottom());

    p.setPen(QColor(55, 55, 65));
    p.drawText(pr.right() - 16, pr.bottom() + 26, QStringLiteral("t"));
    p.drawText(pr.left() - 36, pr.top() + 12, QStringLiteral("δ"));

    // δ reference lines: log10 scale from min to max of the three tolerances
    const double lv0 = std::log10(std::max(m_v0, 1e-15));
    const double lv1 = std::log10(std::max(m_v1, 1e-15));
    const double lve = std::log10(std::max(m_edge, 1e-15));
    double lo = std::min({lv0, lv1, lve});
    double hi = std::max({lv0, lv1, lve});
    if (hi - lo < 0.08)
    {
        hi = lo + 0.5;
    }
    const double lspan = hi - lo;
    auto drawTolLine = [&](double lv, const QString& label, const QColor& col) {
        const double yn = (lv - lo) / lspan;
        const int y = static_cast<int>(std::lround(pr.bottom() - yn * pr.height()));
        if (y < pr.top() || y > pr.bottom())
        {
            return;
        }
        p.setPen(QPen(col, 1, Qt::DotLine));
        p.drawLine(pr.left() + 1, y, pr.right(), y);
        p.setPen(col);
        p.drawText(pr.left() + 10, y - 2, label);
    };
    drawTolLine(lv0, QStringLiteral("v0"), QColor(60, 120, 220));
    drawTolLine(lv1, QStringLiteral("v1"), QColor(200, 110, 50));
    drawTolLine(lve, QStringLiteral("e"), QColor(120, 60, 180));

    // t ticks 0, 0.25, … 1 when visible
    p.setPen(QPen(QColor(210, 210, 218), 1, Qt::DotLine));
    for (int i = 0; i <= 4; ++i)
    {
        const double tv = i * 0.25;
        if (tv < m_t0 - 1e-9 || tv > m_t1 + 1e-9)
        {
            continue;
        }
        const int x = static_cast<int>(std::lround(tToX(tv)));
        p.drawLine(x, pr.bottom() - 4, x, pr.bottom());
    }

    p.setPen(QPen(QColor(70, 70, 80), 2));
    p.drawLine(pr.left(), yMid, pr.right(), yMid);
}

void EdgeSchematicWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    QPainter p(this);
    p.fillRect(rect(), QColor(252, 252, 255));
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRect pr = plotRect();
    if (pr.width() < 8 || pr.height() < 8)
    {
        return;
    }

    const int yMid = pr.center().y();
    drawAxes(p, pr, yMid);

    const double r0 = tolRadiusPx(m_v0);
    const double r1 = tolRadiusPx(m_v1);
    const double re = tolRadiusPx(m_edge);

    auto drawIfVisible = [&](double t, double rad, const QColor& stroke, const QColor& fill) {
        if (t < m_t0 - 1e-12 || t > m_t1 + 1e-12)
        {
            return;
        }
        const double x = tToX(t);
        if (x < pr.left() - rad - 2 || x > pr.right() + rad + 2)
        {
            return;
        }
        p.setPen(QPen(stroke, 1));
        p.setBrush(fill);
        p.drawEllipse(QPointF(x, static_cast<double>(yMid)), rad, rad);
    };

    drawIfVisible(0.0, r0, QColor(30, 80, 160), QColor(60, 120, 220, 200));
    drawIfVisible(1.0, r1, QColor(160, 80, 30), QColor(230, 130, 60, 200));

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(120, 60, 180, 160));
    constexpr int n = 5;
    for (int i = 1; i < n; ++i)
    {
        const double t = static_cast<double>(i) / static_cast<double>(n);
        if (t < m_t0 - 1e-12 || t > m_t1 + 1e-12)
        {
            continue;
        }
        const double x = tToX(t);
        p.drawEllipse(QPointF(x, static_cast<double>(yMid)), re * 0.22, re * 0.22);
    }

    p.setPen(QColor(70, 70, 82));
    p.setBrush(Qt::NoBrush);
    p.drawText(kMarginLeft, height() - 6,
               QStringLiteral("Wheel: zoom t at cursor · Middle drag: pan · Double-click: fit 0–1"));
}

void EdgeSchematicWidget::wheelEvent(QWheelEvent* event)
{
    const int dy = event->angleDelta().y();
    if (dy == 0)
    {
        return;
    }
    const double pivot = xToT(event->position().x());
    const double z = std::pow(kZoomFactorPerNotch, static_cast<double>(dy) / 120.0);
    double a = m_t0;
    double b = m_t1;
    a = pivot - (pivot - a) / z;
    b = pivot + (b - pivot) / z;
    if (b - a < kMinSpan)
    {
        return;
    }
    m_t0 = a;
    m_t1 = b;
    update();
    event->accept();
}

void EdgeSchematicWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton)
    {
        m_panning = true;
        m_lastPan = event->position().toPoint();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void EdgeSchematicWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_panning && (event->buttons() & Qt::MiddleButton))
    {
        const QPoint pt = event->position().toPoint();
        const QRect pr = plotRect();
        const double span = m_t1 - m_t0;
        if (pr.width() > 0)
        {
            const double dt = -static_cast<double>(pt.x() - m_lastPan.x()) / pr.width() * span;
            m_t0 += dt;
            m_t1 += dt;
            update();
        }
        m_lastPan = pt;
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void EdgeSchematicWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton)
    {
        m_panning = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void EdgeSchematicWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    resetTView();
    update();
    event->accept();
}
