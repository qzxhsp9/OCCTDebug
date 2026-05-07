#pragma once

#include <TopoDS_Shape.hxx>

#include <QByteArray>
#include <QPoint>
#include <QPointF>
#include <QWidget>

#include <memory>

class QFocusEvent;
class QMouseEvent;
class QPaintEvent;
class QPaintEngine;
class QResizeEvent;
class QShowEvent;
class QWheelEvent;

#if defined(_WIN32)
struct ViewerOcctData;
#else
class QLabel;
#endif

/// OCCT AIS + V3d view (Windows). Other platforms show a stub message.
///
/// Call refreshPresentation() from the shell when the top-level window is shown/activated or when
/// keyboard focus crosses the viewer so AIS + backing store stay in sync (embedded OCCT contract).
class ViewerWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit ViewerWidget(QWidget* parent = nullptr);
    ~ViewerWidget() override;

    void setRootShape(const TopoDS_Shape& root);
    void setHighlightShape(const TopoDS_Shape& shape);
    void fitAll();

    bool showBoundingBox() const;

public slots:
    /// Resize backing store + redraw (e.g. splitter drag). Does not force AIS update.
    void deferViewportSync();
    /// Push AIS state to the viewer and full redraw — use after window show/activate/focus changes.
    void refreshPresentation();
    void setShowBoundingBox(bool on);

protected:
#if defined(_WIN32)
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
    QPaintEngine* paintEngine() const override;
#endif
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void ensureOcctInitialized();
#if defined(_WIN32)
    void flushView();
    void redrawView();

    void syncOcctViewport();
    void updateBboxAis();
    QPoint mapToOcctPixels(const QPointF& pos) const;

    std::unique_ptr<ViewerOcctData> m_occt;
    TopoDS_Shape m_lastHighlight;
    bool m_inPaintFlush = false;
#endif
    bool m_showBoundingBox = true;
#ifndef _WIN32
    QLabel* m_stubLabel = nullptr;
#endif

    bool m_rotating = false;
    bool m_panning = false;
    QPoint m_lastPos;
};
