#pragma once

#include <QWidget>

class QLabel;
class QStackedWidget;
class QTextBrowser;
class ShapeDocument;
class FaceUvCanvasWidget;
class EdgeSchematicWidget;

/// Topology debug panel: Face (UV / pcurve + zoom), Edge / Vertex (MVP + axes).
class TopologyDetailPanel final : public QWidget
{
    Q_OBJECT

public:
    explicit TopologyDetailPanel(QWidget* parent = nullptr);

    void inspect(const ShapeDocument& document, int shapeId);

private:
    QStackedWidget* m_stack = nullptr;
    QLabel* m_title = nullptr;
    FaceUvCanvasWidget* m_faceCanvas = nullptr;
    QTextBrowser* m_edgeInfo = nullptr;
    EdgeSchematicWidget* m_edgeSchematic = nullptr;
    QTextBrowser* m_vertexInfo = nullptr;
    QLabel* m_placeholder = nullptr;
};
