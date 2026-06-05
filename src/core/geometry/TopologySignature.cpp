#include "core/geometry/TopologySignature.h"

#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <BRepTools.hxx>
#include <GProp_GProps.hxx>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QRegularExpression>
#include <QStringList>
#include <Standard_Failure.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

namespace occtdebug
{
namespace
{
struct GeometryObjectRef
{
    TopAbs_ShapeEnum type = TopAbs_SHAPE;
    int index = 0;
    QString normalizedId;
};

struct SignatureMatchCandidate
{
    int beforeIndex = -1;
    int afterIndex = -1;
    double score = 0.0;
    QString strategy;
};

QString prefixForShapeType(TopAbs_ShapeEnum type)
{
    switch (type)
    {
    case TopAbs_VERTEX:
        return QStringLiteral("V");
    case TopAbs_EDGE:
        return QStringLiteral("E");
    case TopAbs_WIRE:
        return QStringLiteral("W");
    case TopAbs_FACE:
        return QStringLiteral("F");
    case TopAbs_SHELL:
        return QStringLiteral("SHELL");
    case TopAbs_SOLID:
        return QStringLiteral("SOLID");
    case TopAbs_COMPSOLID:
        return QStringLiteral("COMPSOLID");
    case TopAbs_COMPOUND:
        return QStringLiteral("COMPOUND");
    case TopAbs_SHAPE:
        return QStringLiteral("SHAPE");
    }
    return QStringLiteral("SHAPE");
}

QString nameForShapeType(TopAbs_ShapeEnum type)
{
    switch (type)
    {
    case TopAbs_VERTEX:
        return QStringLiteral("vertex");
    case TopAbs_EDGE:
        return QStringLiteral("edge");
    case TopAbs_WIRE:
        return QStringLiteral("wire");
    case TopAbs_FACE:
        return QStringLiteral("face");
    case TopAbs_SHELL:
        return QStringLiteral("shell");
    case TopAbs_SOLID:
        return QStringLiteral("solid");
    case TopAbs_COMPSOLID:
        return QStringLiteral("compsolid");
    case TopAbs_COMPOUND:
        return QStringLiteral("compound");
    case TopAbs_SHAPE:
        return QStringLiteral("shape");
    }
    return QStringLiteral("shape");
}

QString orientationName(const TopAbs_Orientation orientation)
{
    switch (orientation)
    {
    case TopAbs_FORWARD:
        return QStringLiteral("forward");
    case TopAbs_REVERSED:
        return QStringLiteral("reversed");
    case TopAbs_INTERNAL:
        return QStringLiteral("internal");
    case TopAbs_EXTERNAL:
        return QStringLiteral("external");
    }
    return QStringLiteral("unknown");
}

bool parseGeometryObjectId(const QString& raw, GeometryObjectRef* out, QString* error)
{
    const QString value = raw.trimmed();
    if (value.isEmpty())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("geometry object id is empty");
        }
        return false;
    }

    const QRegularExpression pattern(QStringLiteral("^([A-Za-z]+)[_-]?(\\d+)$"));
    const QRegularExpressionMatch match = pattern.match(value);
    if (!match.hasMatch())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("unsupported geometry object id: %1").arg(value);
        }
        return false;
    }

    const QString prefix = match.captured(1).toLower();
    const int index = match.captured(2).toInt();
    if (index <= 0)
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("geometry object index must be positive: %1").arg(value);
        }
        return false;
    }

    TopAbs_ShapeEnum type = TopAbs_SHAPE;
    if (prefix == QStringLiteral("v") || prefix == QStringLiteral("vertex"))
    {
        type = TopAbs_VERTEX;
    }
    else if (prefix == QStringLiteral("e") || prefix == QStringLiteral("edge"))
    {
        type = TopAbs_EDGE;
    }
    else if (prefix == QStringLiteral("w") || prefix == QStringLiteral("wire"))
    {
        type = TopAbs_WIRE;
    }
    else if (prefix == QStringLiteral("f") || prefix == QStringLiteral("face"))
    {
        type = TopAbs_FACE;
    }
    else if (prefix == QStringLiteral("shell"))
    {
        type = TopAbs_SHELL;
    }
    else if (prefix == QStringLiteral("solid"))
    {
        type = TopAbs_SOLID;
    }
    else if (prefix == QStringLiteral("compsolid"))
    {
        type = TopAbs_COMPSOLID;
    }
    else if (prefix == QStringLiteral("compound"))
    {
        type = TopAbs_COMPOUND;
    }
    else if (prefix == QStringLiteral("shape"))
    {
        type = TopAbs_SHAPE;
    }
    else
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("unsupported geometry object prefix: %1").arg(prefix);
        }
        return false;
    }

    if (out != nullptr)
    {
        out->type = type;
        out->index = index;
        out->normalizedId = QStringLiteral("%1%2").arg(prefixForShapeType(type)).arg(index);
    }
    return true;
}

bool findSubShapeByIndex(const TopoDS_Shape& shape, TopAbs_ShapeEnum type, int index, TopoDS_Shape* out)
{
    if (shape.IsNull() || index <= 0)
    {
        return false;
    }

    if (type == TopAbs_SHAPE)
    {
        if (index == 1)
        {
            if (out != nullptr)
            {
                *out = shape;
            }
            return true;
        }
        return false;
    }

    int current = 0;
    for (TopExp_Explorer explorer(shape, type); explorer.More(); explorer.Next())
    {
        ++current;
        if (current == index)
        {
            if (out != nullptr)
            {
                *out = explorer.Current();
            }
            return true;
        }
    }
    return false;
}

QString brepSha256(const TopoDS_Shape& shape)
{
    if (shape.IsNull())
    {
        return QString();
    }

    try
    {
        std::ostringstream stream;
        BRepTools::Write(shape, stream);
        const std::string text = stream.str();
        return QString::fromLatin1(QCryptographicHash::hash(
            QByteArray(text.data(), static_cast<int>(text.size())),
            QCryptographicHash::Sha256).toHex());
    }
    catch (const Standard_Failure&)
    {
        return QString();
    }
}

int subShapeCount(const TopoDS_Shape& shape, TopAbs_ShapeEnum type)
{
    if (shape.IsNull())
    {
        return 0;
    }

    if (type == TopAbs_SHAPE)
    {
        return 1;
    }

    int count = 0;
    for (TopExp_Explorer explorer(shape, type); explorer.More(); explorer.Next())
    {
        ++count;
    }
    return count;
}

QJsonObject subShapeCountsForRecord(const TopoDS_Shape& shape)
{
    return {
        {QStringLiteral("vertex"), subShapeCount(shape, TopAbs_VERTEX)},
        {QStringLiteral("edge"), subShapeCount(shape, TopAbs_EDGE)},
        {QStringLiteral("wire"), subShapeCount(shape, TopAbs_WIRE)},
        {QStringLiteral("face"), subShapeCount(shape, TopAbs_FACE)},
        {QStringLiteral("shell"), subShapeCount(shape, TopAbs_SHELL)},
        {QStringLiteral("solid"), subShapeCount(shape, TopAbs_SOLID)},
        {QStringLiteral("compsolid"), subShapeCount(shape, TopAbs_COMPSOLID)},
        {QStringLiteral("compound"), subShapeCount(shape, TopAbs_COMPOUND)},
    };
}

QJsonObject pointToJson(double x, double y, double z)
{
    return {
        {QStringLiteral("x"), x},
        {QStringLiteral("y"), y},
        {QStringLiteral("z"), z},
    };
}

QString measureKind(TopAbs_ShapeEnum type)
{
    switch (type)
    {
    case TopAbs_EDGE:
    case TopAbs_WIRE:
        return QStringLiteral("length");
    case TopAbs_FACE:
    case TopAbs_SHELL:
        return QStringLiteral("area");
    case TopAbs_SOLID:
    case TopAbs_COMPSOLID:
    case TopAbs_COMPOUND:
    case TopAbs_SHAPE:
        return QStringLiteral("volume_or_mass");
    case TopAbs_VERTEX:
        return QStringLiteral("point");
    }
    return QStringLiteral("unknown");
}

QJsonObject geometryDescriptor(const TopoDS_Shape& shape, TopAbs_ShapeEnum type)
{
    QJsonObject geometry {
        {QStringLiteral("measure_kind"), measureKind(type)},
    };
    if (shape.IsNull())
    {
        geometry.insert(QStringLiteral("available"), false);
        return geometry;
    }

    try
    {
        Bnd_Box box;
        BRepBndLib::Add(shape, box);
        if (!box.IsVoid())
        {
            Standard_Real xmin = 0.0;
            Standard_Real ymin = 0.0;
            Standard_Real zmin = 0.0;
            Standard_Real xmax = 0.0;
            Standard_Real ymax = 0.0;
            Standard_Real zmax = 0.0;
            box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
            const double dx = xmax - xmin;
            const double dy = ymax - ymin;
            const double dz = zmax - zmin;
            geometry.insert(QStringLiteral("bbox"), QJsonObject {
                {QStringLiteral("min"), pointToJson(xmin, ymin, zmin)},
                {QStringLiteral("max"), pointToJson(xmax, ymax, zmax)},
                {QStringLiteral("size"), pointToJson(dx, dy, dz)},
                {QStringLiteral("diagonal"), std::sqrt(dx * dx + dy * dy + dz * dz)},
            });
            geometry.insert(QStringLiteral("bbox_center"), pointToJson(
                (xmin + xmax) * 0.5,
                (ymin + ymax) * 0.5,
                (zmin + zmax) * 0.5));
        }

        GProp_GProps props;
        if (type == TopAbs_EDGE || type == TopAbs_WIRE)
        {
            BRepGProp::LinearProperties(shape, props);
        }
        else if (type == TopAbs_FACE || type == TopAbs_SHELL)
        {
            BRepGProp::SurfaceProperties(shape, props);
        }
        else if (type != TopAbs_VERTEX)
        {
            BRepGProp::VolumeProperties(shape, props);
        }
        const double measure = props.Mass();
        if (std::isfinite(measure))
        {
            geometry.insert(QStringLiteral("measure"), measure);
        }
        const gp_Pnt center = props.CentreOfMass();
        geometry.insert(QStringLiteral("center_of_mass"), pointToJson(center.X(), center.Y(), center.Z()));
        geometry.insert(QStringLiteral("available"), true);
    }
    catch (const Standard_Failure&)
    {
        geometry.insert(QStringLiteral("available"), false);
    }
    return geometry;
}

QJsonObject makeRecord(const QString& objectId,
                       TopAbs_ShapeEnum type,
                       int index,
                       const TopoDS_Shape& shape)
{
    const QString hash = brepSha256(shape);
    return {
        {QStringLiteral("object_id"), objectId},
        {QStringLiteral("stable_id"), hash.isEmpty() ? objectId : QStringLiteral("%1#%2").arg(objectId, hash.left(16))},
        {QStringLiteral("type"), nameForShapeType(type)},
        {QStringLiteral("index"), index},
        {QStringLiteral("brep_sha256"), hash},
        {QStringLiteral("orientation"), shape.IsNull() ? QStringLiteral("null") : orientationName(shape.Orientation())},
        {QStringLiteral("children"), shape.IsNull() ? 0 : shape.NbChildren()},
        {QStringLiteral("subshape_counts"), shape.IsNull() ? QJsonObject {} : subShapeCountsForRecord(shape)},
        {QStringLiteral("geometry"), geometryDescriptor(shape, type)},
    };
}

QJsonArray recordsForType(const TopoDS_Shape& rootShape, TopAbs_ShapeEnum type)
{
    QJsonArray records;
    if (rootShape.IsNull())
    {
        return records;
    }

    if (type == TopAbs_SHAPE)
    {
        records.append(makeRecord(QStringLiteral("SHAPE1"), type, 1, rootShape));
        return records;
    }

    int index = 0;
    for (TopExp_Explorer explorer(rootShape, type); explorer.More(); explorer.Next())
    {
        ++index;
        const QString objectId = QStringLiteral("%1%2").arg(prefixForShapeType(type)).arg(index);
        records.append(makeRecord(objectId, type, index, explorer.Current()));
    }
    return records;
}

QJsonArray recordsFromSignature(const QJsonObject& signature)
{
    return signature.value(QStringLiteral("records")).toArray();
}

QString recordKey(const QJsonObject& record)
{
    return record.value(QStringLiteral("object_id")).toString();
}

double numericValue(const QJsonObject& object, const QString& key, double fallback = 0.0)
{
    const QJsonValue value = object.value(key);
    return value.isDouble() ? value.toDouble() : fallback;
}

double relativeSimilarity(double before, double after, double tolerance = 1.0e-7)
{
    if (!std::isfinite(before) || !std::isfinite(after))
    {
        return 0.0;
    }
    const double scale = std::max({std::abs(before), std::abs(after), tolerance});
    const double relative = std::abs(before - after) / scale;
    return std::clamp(1.0 - relative, 0.0, 1.0);
}

double pointSimilarity(const QJsonObject& before, const QJsonObject& after, double scale)
{
    const double dx = numericValue(before, QStringLiteral("x")) - numericValue(after, QStringLiteral("x"));
    const double dy = numericValue(before, QStringLiteral("y")) - numericValue(after, QStringLiteral("y"));
    const double dz = numericValue(before, QStringLiteral("z")) - numericValue(after, QStringLiteral("z"));
    const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    const double denominator = std::max(scale, 1.0e-7);
    return std::clamp(1.0 - distance / denominator, 0.0, 1.0);
}

double geometrySimilarity(const QJsonObject& beforeRecord, const QJsonObject& afterRecord, QStringList* hints)
{
    const QJsonObject beforeGeometry = beforeRecord.value(QStringLiteral("geometry")).toObject();
    const QJsonObject afterGeometry = afterRecord.value(QStringLiteral("geometry")).toObject();
    if (!beforeGeometry.value(QStringLiteral("available")).toBool()
        || !afterGeometry.value(QStringLiteral("available")).toBool())
    {
        return 0.0;
    }

    double score = 0.0;
    double weight = 0.0;
    const double measureScore = relativeSimilarity(beforeGeometry.value(QStringLiteral("measure")).toDouble(0.0),
                                                   afterGeometry.value(QStringLiteral("measure")).toDouble(0.0));
    if (beforeGeometry.contains(QStringLiteral("measure")) && afterGeometry.contains(QStringLiteral("measure")))
    {
        score += measureScore * 0.45;
        weight += 0.45;
        if (measureScore > 0.98 && hints != nullptr)
        {
            hints->append(QStringLiteral("same_measure"));
        }
    }

    const QJsonObject beforeBox = beforeGeometry.value(QStringLiteral("bbox")).toObject();
    const QJsonObject afterBox = afterGeometry.value(QStringLiteral("bbox")).toObject();
    const double beforeDiagonal = beforeBox.value(QStringLiteral("diagonal")).toDouble(0.0);
    const double afterDiagonal = afterBox.value(QStringLiteral("diagonal")).toDouble(0.0);
    if (beforeDiagonal > 0.0 || afterDiagonal > 0.0)
    {
        const double diagonalScore = relativeSimilarity(beforeDiagonal, afterDiagonal);
        score += diagonalScore * 0.30;
        weight += 0.30;
        if (diagonalScore > 0.98 && hints != nullptr)
        {
            hints->append(QStringLiteral("same_bbox_diagonal"));
        }
    }

    if (beforeGeometry.contains(QStringLiteral("bbox_center")) && afterGeometry.contains(QStringLiteral("bbox_center")))
    {
        const double scale = std::max({beforeDiagonal, afterDiagonal, 1.0e-7});
        const double centerScore = pointSimilarity(beforeGeometry.value(QStringLiteral("bbox_center")).toObject(),
                                                   afterGeometry.value(QStringLiteral("bbox_center")).toObject(),
                                                   scale);
        score += centerScore * 0.25;
        weight += 0.25;
        if (centerScore > 0.98 && hints != nullptr)
        {
            hints->append(QStringLiteral("same_bbox_center"));
        }
    }

    return weight > 0.0 ? score / weight : 0.0;
}

double subshapeCountSimilarity(const QJsonObject& beforeRecord, const QJsonObject& afterRecord, QStringList* hints)
{
    const QJsonObject beforeCounts = beforeRecord.value(QStringLiteral("subshape_counts")).toObject();
    const QJsonObject afterCounts = afterRecord.value(QStringLiteral("subshape_counts")).toObject();
    if (beforeCounts.isEmpty() || afterCounts.isEmpty())
    {
        return 0.0;
    }

    const QStringList keys {
        QStringLiteral("vertex"),
        QStringLiteral("edge"),
        QStringLiteral("wire"),
        QStringLiteral("face"),
        QStringLiteral("shell"),
        QStringLiteral("solid"),
        QStringLiteral("compsolid"),
        QStringLiteral("compound"),
    };
    double score = 0.0;
    int used = 0;
    for (const QString& key : keys)
    {
        const int before = beforeCounts.value(key).toInt(-1);
        const int after = afterCounts.value(key).toInt(-2);
        if (before < 0 || after < 0)
        {
            continue;
        }
        const double scale = std::max({std::abs(before), std::abs(after), 1});
        score += 1.0 - (std::abs(before - after) / scale);
        ++used;
    }
    const double normalized = used > 0 ? std::clamp(score / used, 0.0, 1.0) : 0.0;
    if (normalized > 0.98 && hints != nullptr)
    {
        hints->append(QStringLiteral("same_subshape_counts"));
    }
    return normalized;
}

SignatureMatchCandidate scoreCandidate(const QJsonObject& beforeRecord,
                                       const QJsonObject& afterRecord,
                                       int beforeIndex,
                                       int afterIndex)
{
    SignatureMatchCandidate candidate;
    candidate.beforeIndex = beforeIndex;
    candidate.afterIndex = afterIndex;

    const QString beforeType = beforeRecord.value(QStringLiteral("type")).toString();
    const QString afterType = afterRecord.value(QStringLiteral("type")).toString();
    if (beforeType.isEmpty() || beforeType != afterType)
    {
        return candidate;
    }

    const QString beforeHash = beforeRecord.value(QStringLiteral("brep_sha256")).toString();
    const QString afterHash = afterRecord.value(QStringLiteral("brep_sha256")).toString();
    if (!beforeHash.isEmpty() && beforeHash == afterHash)
    {
        candidate.score = 1.0;
        candidate.strategy = QStringLiteral("exact_brep_sha256");
        return candidate;
    }

    double score = 0.25; // Same subshape type is required before weaker topology hints are considered.
    QStringList hints {QStringLiteral("same_type")};

    if (recordKey(beforeRecord) == recordKey(afterRecord))
    {
        score += 0.20;
        hints.append(QStringLiteral("same_object_id"));
    }
    if (beforeRecord.value(QStringLiteral("orientation")).toString()
        == afterRecord.value(QStringLiteral("orientation")).toString())
    {
        score += 0.15;
        hints.append(QStringLiteral("same_orientation"));
    }
    if (beforeRecord.value(QStringLiteral("children")).toInt(-1)
        == afterRecord.value(QStringLiteral("children")).toInt(-2))
    {
        score += 0.15;
        hints.append(QStringLiteral("same_children"));
    }
    if (beforeRecord.value(QStringLiteral("index")).toInt(-1)
        == afterRecord.value(QStringLiteral("index")).toInt(-2))
    {
        score += 0.10;
        hints.append(QStringLiteral("same_index"));
    }

    const double childTopologyScore = subshapeCountSimilarity(beforeRecord, afterRecord, &hints);
    if (childTopologyScore > 0.0)
    {
        score += childTopologyScore * 0.15;
    }

    const double localGeometryScore = geometrySimilarity(beforeRecord, afterRecord, &hints);
    if (localGeometryScore > 0.0)
    {
        score += localGeometryScore * 0.20;
    }

    candidate.score = std::min(score, 0.95);
    candidate.strategy = hints.join(QLatin1Char('+'));
    return candidate;
}

QJsonObject countsDelta(const QJsonObject& beforeCounts, const QJsonObject& afterCounts)
{
    QJsonObject delta;
    const QStringList keys {
        QStringLiteral("shape"),
        QStringLiteral("vertex"),
        QStringLiteral("edge"),
        QStringLiteral("wire"),
        QStringLiteral("face"),
        QStringLiteral("shell"),
        QStringLiteral("solid"),
        QStringLiteral("compsolid"),
        QStringLiteral("compound"),
    };
    for (const QString& key : keys)
    {
        const int before = beforeCounts.value(key).toInt();
        const int after = afterCounts.value(key).toInt();
        delta.insert(key, after - before);
    }
    return delta;
}

QJsonObject compactRecordRef(const QJsonObject& record)
{
    return {
        {QStringLiteral("object_id"), record.value(QStringLiteral("object_id")).toString()},
        {QStringLiteral("stable_id"), record.value(QStringLiteral("stable_id")).toString()},
        {QStringLiteral("type"), record.value(QStringLiteral("type")).toString()},
        {QStringLiteral("index"), record.value(QStringLiteral("index")).toInt()},
        {QStringLiteral("brep_sha256"), record.value(QStringLiteral("brep_sha256")).toString()},
        {QStringLiteral("geometry"), record.value(QStringLiteral("geometry")).toObject()},
    };
}
} // namespace

QJsonObject TopologySignature::build(const TopoDS_Shape& shape,
                                     const QString& sourceLabel,
                                     const QString& selectedObjectId)
{
    QJsonObject counts {
        {QStringLiteral("shape"), subShapeCount(shape, TopAbs_SHAPE)},
        {QStringLiteral("vertex"), subShapeCount(shape, TopAbs_VERTEX)},
        {QStringLiteral("edge"), subShapeCount(shape, TopAbs_EDGE)},
        {QStringLiteral("wire"), subShapeCount(shape, TopAbs_WIRE)},
        {QStringLiteral("face"), subShapeCount(shape, TopAbs_FACE)},
        {QStringLiteral("shell"), subShapeCount(shape, TopAbs_SHELL)},
        {QStringLiteral("solid"), subShapeCount(shape, TopAbs_SOLID)},
        {QStringLiteral("compsolid"), subShapeCount(shape, TopAbs_COMPSOLID)},
        {QStringLiteral("compound"), subShapeCount(shape, TopAbs_COMPOUND)},
    };

    QJsonArray records;
    const auto appendRecords = [&records](const QJsonArray& next) {
        for (const QJsonValue& value : next)
        {
            records.append(value);
        }
    };
    appendRecords(recordsForType(shape, TopAbs_SHAPE));
    appendRecords(recordsForType(shape, TopAbs_VERTEX));
    appendRecords(recordsForType(shape, TopAbs_EDGE));
    appendRecords(recordsForType(shape, TopAbs_WIRE));
    appendRecords(recordsForType(shape, TopAbs_FACE));
    appendRecords(recordsForType(shape, TopAbs_SHELL));
    appendRecords(recordsForType(shape, TopAbs_SOLID));
    appendRecords(recordsForType(shape, TopAbs_COMPSOLID));
    appendRecords(recordsForType(shape, TopAbs_COMPOUND));

    QJsonObject root {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("basis"), QStringLiteral("TopExp traversal object_id plus SHA-256 of BRepTools::Write(subshape), local geometry descriptor, and subshape count topology hints")},
        {QStringLiteral("source"), sourceLabel},
        {QStringLiteral("counts"), counts},
        {QStringLiteral("records"), records},
    };
    if (!selectedObjectId.trimmed().isEmpty())
    {
        QString error;
        const QJsonObject selected = objectSignature(shape, selectedObjectId, &error);
        root.insert(QStringLiteral("selected_object"), selected.isEmpty()
            ? QJsonObject {{QStringLiteral("object_id"), selectedObjectId}, {QStringLiteral("error"), error}}
            : selected);
    }
    return root;
}

QJsonObject TopologySignature::objectSignature(const TopoDS_Shape& rootShape,
                                               const QString& objectId,
                                               QString* error)
{
    if (rootShape.IsNull())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("no shape loaded");
        }
        return {};
    }

    GeometryObjectRef ref;
    if (!parseGeometryObjectId(objectId, &ref, error))
    {
        return {};
    }

    TopoDS_Shape subShape;
    if (!findSubShapeByIndex(rootShape, ref.type, ref.index, &subShape) || subShape.IsNull())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("%1 was not found in the current topology").arg(ref.normalizedId);
        }
        return {};
    }
    return makeRecord(ref.normalizedId, ref.type, ref.index, subShape);
}

QString TopologySignature::stableIdForObject(const TopoDS_Shape& rootShape,
                                             const QString& objectId,
                                             QString* error)
{
    const QJsonObject signature = objectSignature(rootShape, objectId, error);
    return signature.value(QStringLiteral("stable_id")).toString();
}

QJsonObject TopologySignature::compare(const QJsonObject& beforeSignature,
                                       const QJsonObject& afterSignature,
                                       double minimumScore)
{
    const QJsonArray beforeRecords = recordsFromSignature(beforeSignature);
    const QJsonArray afterRecords = recordsFromSignature(afterSignature);
    const double threshold = std::clamp(minimumScore, 0.0, 1.0);

    std::vector<SignatureMatchCandidate> candidates;
    candidates.reserve(static_cast<size_t>(beforeRecords.size() * afterRecords.size()));
    for (int beforeIndex = 0; beforeIndex < beforeRecords.size(); ++beforeIndex)
    {
        const QJsonObject beforeRecord = beforeRecords.at(beforeIndex).toObject();
        for (int afterIndex = 0; afterIndex < afterRecords.size(); ++afterIndex)
        {
            const QJsonObject afterRecord = afterRecords.at(afterIndex).toObject();
            SignatureMatchCandidate candidate = scoreCandidate(beforeRecord, afterRecord, beforeIndex, afterIndex);
            if (candidate.score >= threshold)
            {
                candidates.push_back(candidate);
            }
        }
    }

    std::sort(candidates.begin(), candidates.end(), [beforeRecords, afterRecords](const SignatureMatchCandidate& left,
                                                                                  const SignatureMatchCandidate& right) {
        if (left.score != right.score)
        {
            return left.score > right.score;
        }
        const QString leftBefore = recordKey(beforeRecords.at(left.beforeIndex).toObject());
        const QString rightBefore = recordKey(beforeRecords.at(right.beforeIndex).toObject());
        if (leftBefore != rightBefore)
        {
            return leftBefore < rightBefore;
        }
        return recordKey(afterRecords.at(left.afterIndex).toObject())
            < recordKey(afterRecords.at(right.afterIndex).toObject());
    });

    std::vector<bool> beforeUsed(static_cast<size_t>(beforeRecords.size()), false);
    std::vector<bool> afterUsed(static_cast<size_t>(afterRecords.size()), false);
    QJsonArray matches;
    int exactMatches = 0;
    int approximateMatches = 0;
    for (const SignatureMatchCandidate& candidate : candidates)
    {
        if (beforeUsed.at(static_cast<size_t>(candidate.beforeIndex))
            || afterUsed.at(static_cast<size_t>(candidate.afterIndex)))
        {
            continue;
        }

        beforeUsed[static_cast<size_t>(candidate.beforeIndex)] = true;
        afterUsed[static_cast<size_t>(candidate.afterIndex)] = true;
        if (candidate.score >= 1.0)
        {
            ++exactMatches;
        }
        else
        {
            ++approximateMatches;
        }

        matches.append(QJsonObject {
            {QStringLiteral("before"), compactRecordRef(beforeRecords.at(candidate.beforeIndex).toObject())},
            {QStringLiteral("after"), compactRecordRef(afterRecords.at(candidate.afterIndex).toObject())},
            {QStringLiteral("score"), candidate.score},
            {QStringLiteral("strategy"), candidate.strategy},
        });
    }

    QJsonArray unmatchedBefore;
    for (int index = 0; index < beforeRecords.size(); ++index)
    {
        if (!beforeUsed.at(static_cast<size_t>(index)))
        {
            unmatchedBefore.append(compactRecordRef(beforeRecords.at(index).toObject()));
        }
    }

    QJsonArray unmatchedAfter;
    for (int index = 0; index < afterRecords.size(); ++index)
    {
        if (!afterUsed.at(static_cast<size_t>(index)))
        {
            unmatchedAfter.append(compactRecordRef(afterRecords.at(index).toObject()));
        }
    }

    QString status = QStringLiteral("stable");
    if (!unmatchedBefore.isEmpty() || !unmatchedAfter.isEmpty())
    {
        status = QStringLiteral("changed");
    }
    else if (approximateMatches > 0)
    {
        status = QStringLiteral("renamed_or_modified");
    }

    const QJsonObject summary {
        {QStringLiteral("status"), status},
        {QStringLiteral("before_records"), beforeRecords.size()},
        {QStringLiteral("after_records"), afterRecords.size()},
        {QStringLiteral("matched"), matches.size()},
        {QStringLiteral("exact_hash_matches"), exactMatches},
        {QStringLiteral("approximate_matches"), approximateMatches},
        {QStringLiteral("unmatched_before"), unmatchedBefore.size()},
        {QStringLiteral("unmatched_after"), unmatchedAfter.size()},
    };

    return {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("basis"), QStringLiteral("Greedy same-type matching: exact BREP SHA-256 first, then object_id/orientation/children/index plus local geometry and subshape count score")},
        {QStringLiteral("minimum_score"), threshold},
        {QStringLiteral("before_source"), beforeSignature.value(QStringLiteral("source")).toString()},
        {QStringLiteral("after_source"), afterSignature.value(QStringLiteral("source")).toString()},
        {QStringLiteral("counts_delta"), countsDelta(beforeSignature.value(QStringLiteral("counts")).toObject(),
                                                     afterSignature.value(QStringLiteral("counts")).toObject())},
        {QStringLiteral("summary"), summary},
        {QStringLiteral("matches"), matches},
        {QStringLiteral("unmatched_before"), unmatchedBefore},
        {QStringLiteral("unmatched_after"), unmatchedAfter},
    };
}
} // namespace occtdebug
