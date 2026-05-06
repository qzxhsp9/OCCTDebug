#pragma once

#include "core/ShapeDocument.h"

#include <QByteArray>

class ShapeTreeJsonExporter
{
public:
    static QByteArray exportDocument(const ShapeDocument& document);
};
