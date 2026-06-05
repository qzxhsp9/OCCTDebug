#pragma once

#include <QJsonObject>
#include <QWidget>

#include <AIS_InteractiveContext.hxx>
#include <TopoDS_Shape.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

class QPaintEngine;
class QMouseEvent;
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
    void clearView();
    void displayDemoShape();
    bool loadModelFile(const QString& filePath, QString* error = nullptr);
    bool highlightGeometryObject(const QString& objectId, QString* error = nullptr);
    void clearHighlight();

public:
    QString highlightedObjectId() const;
    QString topologySummary() const;
    QString topologySignatureForObject(const QString& objectId, QString* error = nullptr) const;
    QJsonObject topologySignatureJson(const QString& sourceLabel = QString(),
                                      const QString& selectedObjectId = QString()) const;
    bool saveScreenshot(const QString& filePath, QString* error = nullptr);

signals:
    void geometryObjectPicked(const QString& objectId, const QString& summary);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void initializeViewer();
    void displayShape(const TopoDS_Shape& shape);
    QString objectIdForShape(const TopoDS_Shape& shape) const;
    void activateSubshapeSelection(const Handle(AIS_InteractiveObject)& object);

    Handle(V3d_Viewer) m_viewer;
    Handle(V3d_View) m_view;
    Handle(AIS_InteractiveContext) m_context;
    Handle(AIS_InteractiveObject) m_displayedShape;
    Handle(AIS_InteractiveObject) m_highlightedShape;
    TopoDS_Shape m_currentShape;
    QString m_highlightedObjectId;
    bool m_initialized = false;
};
} // namespace occtdebug
