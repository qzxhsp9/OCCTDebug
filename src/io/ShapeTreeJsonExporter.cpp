#include "io/ShapeTreeJsonExporter.h"

#include "occt/OcctUtils.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

QByteArray ShapeTreeJsonExporter::exportDocument(const ShapeDocument& document)
{
    QJsonArray arr;
    for (const ShapeNode& n : document.Nodes())
    {
        QJsonObject o;
        o[QStringLiteral("id")] = n.id;
        o[QStringLiteral("parentId")] = n.parentId;
        o[QStringLiteral("name")] = QString::fromStdString(n.name);
        o[QStringLiteral("kind")] = QString::fromLatin1(ShapeKindDisplayName(n.kind));
        o[QStringLiteral("tolerance")] = n.tolerance;
        o[QStringLiteral("isNull")] = n.isNull;
        o[QStringLiteral("isClosed")] = n.isClosed;
        QJsonArray ch;
        for (int c : n.children)
        {
            ch.append(c);
        }
        o[QStringLiteral("children")] = ch;

        if (!n.bbox.IsVoid())
        {
            double xmin = 0, ymin = 0, zmin = 0, xmax = 0, ymax = 0, zmax = 0;
            n.bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
            QJsonObject bb;
            bb[QStringLiteral("xmin")] = xmin;
            bb[QStringLiteral("ymin")] = ymin;
            bb[QStringLiteral("zmin")] = zmin;
            bb[QStringLiteral("xmax")] = xmax;
            bb[QStringLiteral("ymax")] = ymax;
            bb[QStringLiteral("zmax")] = zmax;
            o[QStringLiteral("bbox")] = bb;
        }

        arr.append(o);
    }

    QJsonDocument doc(arr);
    return doc.toJson(QJsonDocument::Indented);
}
