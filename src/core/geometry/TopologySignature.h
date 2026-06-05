#pragma once

#include <QJsonObject>
#include <QString>

#include <TopoDS_Shape.hxx>

namespace occtdebug
{
class TopologySignature
{
public:
    static QJsonObject build(const TopoDS_Shape& shape,
                             const QString& sourceLabel = QString(),
                             const QString& selectedObjectId = QString());
    static QJsonObject objectSignature(const TopoDS_Shape& rootShape,
                                       const QString& objectId,
                                       QString* error = nullptr);
    static QString stableIdForObject(const TopoDS_Shape& rootShape,
                                     const QString& objectId,
                                     QString* error = nullptr);
    static QJsonObject compare(const QJsonObject& beforeSignature,
                               const QJsonObject& afterSignature,
                               double minimumScore = 0.50);
};
} // namespace occtdebug
