#pragma once

#include <QWidget>

#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

class QPaintEngine;
class QResizeEvent;
class QShowEvent;

namespace occtdebug
{
class OcctViewerWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit OcctViewerWidget(QWidget* parent = nullptr);

    QPaintEngine* paintEngine() const override;
    QSize minimumSizeHint() const override;

public slots:
    void fitAll();
    void displayDemoShape();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void initializeViewer();

    Handle(V3d_Viewer) m_viewer;
    Handle(V3d_View) m_view;
    Handle(AIS_InteractiveContext) m_context;
    bool m_initialized = false;
};
} // namespace occtdebug
