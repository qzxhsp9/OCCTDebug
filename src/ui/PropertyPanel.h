#pragma once

#include "core/ShapeDocument.h"

#include <QWidget>

class QTextBrowser;

class PropertyPanel final : public QWidget
{
    Q_OBJECT

public:
    explicit PropertyPanel(QWidget* parent = nullptr);

    void showShape(const ShapeDocument& document, int shapeId);

private:
    QTextBrowser* m_text = nullptr;
};
