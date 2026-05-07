#pragma once

#include <QPoint>
#include <QWidget>

/// Schematic: edge as a line in **t** (curve parameter 0–1), tolerances as disks; **δ** axis (log
/// scale ticks); wheel zooms **at cursor** along t, middle-drag pans, double-click fits.
class EdgeSchematicWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit EdgeSchematicWidget(QWidget* parent = nullptr);

    void setTolerances(double vertexTol0, double vertexTol1, double edgeTol);

protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void resetTView();
    QRect plotRect() const;
    double xToT(double px) const;
    double tToX(double t) const;
    void drawAxes(QPainter& p, const QRect& pr, int yMid) const;

    double m_v0 = 1e-7;
    double m_v1 = 1e-7;
    double m_edge = 1e-7;

    double m_t0 = 0.0;
    double m_t1 = 1.0;
    bool m_panning = false;
    QPoint m_lastPan;
};
