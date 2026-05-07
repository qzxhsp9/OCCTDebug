#pragma once

#include "occt/FaceUvExtractor.h"

#include <QPoint>
#include <QWidget>

#include <vector>

/// Face (u,v) parameter domain: polylines, **u/v coordinate axes**, wheel zoom **at cursor**, middle-drag pan.
class FaceUvCanvasWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit FaceUvCanvasWidget(QWidget* parent = nullptr);

    void setPolylines(const std::vector<FaceUvPolyline>& polys);

protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void recomputeDataBounds();
    void fitViewToData();
    QRect plotRect() const;
    QPointF widgetToUv(QPointF px) const;
    QPointF uvToWidget(double u, double v) const;
    void drawAxesAndGrid(QPainter& g, const QRect& pr) const;

    std::vector<FaceUvPolyline> m_polys;
    bool m_hasData = false;
    double m_du0 = 0.0;
    double m_du1 = 1.0;
    double m_dv0 = 0.0;
    double m_dv1 = 1.0;

    double m_vu0 = 0.0;
    double m_vu1 = 1.0;
    double m_vv0 = 0.0;
    double m_vv1 = 1.0;

    bool m_panning = false;
    QPoint m_lastPan;
};
