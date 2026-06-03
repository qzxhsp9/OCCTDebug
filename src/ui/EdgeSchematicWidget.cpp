#include "ui/EdgeSchematicWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace
{
constexpr int kMarginLeft = 36;
constexpr int kMarginRight = 8;
constexpr int kMarginTop = 8;
constexpr int kMarginBottom = 24;
constexpr double kZoomFactorPerNotch = 1.14;
constexpr double kMinSpan = 1e-6;

double tolRadiusPx(double tol)
{
    return std::clamp(-std::log10(std::max(tol, 1e-12)) * 3.0, 5.0, 38.0);
}

double logTol(double tol)
{
    return std::log10(std::max(tol, 1e-15));
}

void toleranceLogRange(double v0, double v1, double edge, double& lo, double& hi)
{
    lo = std::min({logTol(v0), logTol(v1), logTol(edge)});
    hi = std::max({logTol(v0), logTol(v1), logTol(edge)});
    if (hi - lo < 0.08)
    {
        hi = lo + 0.5;
    }
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

double EdgeSchematicWidget::toleranceToY(double tol, const QRect& pr) const
{
    double lo = 0.0;
    double hi = 1.0;
    toleranceLogRange(m_v0, m_v1, m_edge, lo, hi);
    const double yn = (logTol(tol) - lo) / (hi - lo);
    return pr.bottom() - yn * pr.height();
}

void EdgeSchematicWidget::drawAxes(QPainter& p, const QRect& pr) const
{
    p.setPen(QPen(QColor(130, 130, 140), 1));
    p.drawLine(pr.left(), pr.bottom(), pr.right(), pr.bottom());
    p.drawLine(pr.left(), pr.top(), pr.left(), pr.bottom());

    p.setPen(QColor(55, 55, 65));
    p.drawText(pr.right() - 16, pr.bottom() + 26, QStringLiteral("t"));
    p.drawText(pr.left() - 34, pr.top() + 12, QStringLiteral("tol"));

    auto drawTolLine = [&](double tol, const QString& label, const QColor& col) {
        const int y = static_cast<int>(std::lround(toleranceToY(tol, pr)));
        if (y < pr.top() || y > pr.bottom())
        {
            return;
        }
        p.setPen(QPen(col, 1, Qt::DotLine));
        p.drawLine(pr.left() + 1, y, pr.right(), y);
        p.setPen(col);
        p.drawText(pr.left() + 10, y - 2, label);
    };
    drawTolLine(m_v0, QStringLiteral("v0"), QColor(60, 120, 220));
    drawTolLine(m_v1, QStringLiteral("v1"), QColor(200, 110, 50));
    drawTolLine(m_edge, QStringLiteral("e"), QColor(120, 60, 180));

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

    const int yEdge = static_cast<int>(std::lround(toleranceToY(m_edge, pr)));
    p.setPen(QPen(QColor(70, 70, 80), 2));
    p.drawLine(pr.left(), yEdge, pr.right(), yEdge);
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

    drawAxes(p, pr);

    const double r0 = tolRadiusPx(m_v0);
    const double r1 = tolRadiusPx(m_v1);
    const double re = tolRadiusPx(m_edge);

    auto drawIfVisible = [&](double t, double tol, double rad, const QColor& stroke, const QColor& fill) {
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
        p.drawEllipse(QPointF(x, toleranceToY(tol, pr)), rad, rad);
    };

    drawIfVisible(0.0, m_v0, r0, QColor(30, 80, 160), QColor(60, 120, 220, 200));
    drawIfVisible(1.0, m_v1, r1, QColor(160, 80, 30), QColor(230, 130, 60, 200));

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
        p.drawEllipse(QPointF(x, toleranceToY(m_edge, pr)), re * 0.22, re * 0.22);
    }

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
