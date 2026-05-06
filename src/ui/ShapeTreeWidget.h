#pragma once

#include "core/ShapeDocument.h"

#include <QTreeWidget>

class ShapeTreeWidget final : public QTreeWidget
{
    Q_OBJECT

public:
    explicit ShapeTreeWidget(QWidget* parent = nullptr);

    void rebuildFromDocument(const ShapeDocument& document);

    void selectShapeId(int shapeId);

signals:
    void shapeSelected(int shapeId);

private slots:
    void onCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);

private:
    QHash<int, QTreeWidgetItem*> m_idToItem;
};
