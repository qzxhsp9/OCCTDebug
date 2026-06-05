#include "core/verify/TestdiffArtifactIndex.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QStringList>

namespace occtdebug
{
namespace
{
struct ArtifactSlot
{
    QString path;
    double bytes = 0.0;
    bool present = false;
};

struct ArtifactGroup
{
    QString kind;
    QString key;
    ArtifactSlot before;
    ArtifactSlot after;
    ArtifactSlot diff;
};

QString normalizedPath(QString value)
{
    return value.replace(QLatin1Char('\\'), QLatin1Char('/'));
}

QStringList supportedKinds()
{
    return {
        QStringLiteral("image"),
        QStringLiteral("property"),
        QStringLiteral("performance"),
    };
}

QJsonArray supportedKindsJson()
{
    QJsonArray out;
    for (const QString& kind : supportedKinds())
    {
        out.append(kind);
    }
    return out;
}

QJsonObject emptyRoleCounts()
{
    return {
        {QStringLiteral("before"), 0},
        {QStringLiteral("after"), 0},
        {QStringLiteral("diff"), 0},
        {QStringLiteral("other"), 0},
        {QStringLiteral("total"), 0},
    };
}

QJsonObject initialCounts()
{
    QJsonObject out;
    for (const QString& kind : supportedKinds())
    {
        out.insert(kind, emptyRoleCounts());
    }
    return out;
}

void incrementKindRole(QJsonObject& counts, const QString& kind, const QString& role)
{
    QJsonObject roleCounts = counts.value(kind).toObject(emptyRoleCounts());
    const QString normalizedRole = (role == QStringLiteral("before")
                                    || role == QStringLiteral("after")
                                    || role == QStringLiteral("diff"))
        ? role
        : QStringLiteral("other");
    roleCounts.insert(normalizedRole, roleCounts.value(normalizedRole).toInt() + 1);
    roleCounts.insert(QStringLiteral("total"), roleCounts.value(QStringLiteral("total")).toInt() + 1);
    counts.insert(kind, roleCounts);
}

QString stripRolePrefix(const QString& path, const QString& role)
{
    const QString normalized = normalizedPath(path);
    const QString lower = normalized.toLower();
    const QString lowerRole = role.toLower();
    const QStringList prefixes {
        QStringLiteral("artifacts/testdiff/%1/").arg(lowerRole),
        QStringLiteral("verification/testdiff/%1/").arg(lowerRole),
        QStringLiteral("artifacts/testdiff_%1/").arg(lowerRole),
        QStringLiteral("verification/testdiff_%1/").arg(lowerRole),
        QStringLiteral("%1/").arg(lowerRole),
    };

    for (const QString& prefix : prefixes)
    {
        if (lower.startsWith(prefix))
        {
            return normalized.mid(prefix.size());
        }
    }
    return normalized;
}

QString artifactKey(const QString& path, const QString& role)
{
    const QString stripped = stripRolePrefix(path, role);
    const QFileInfo info(stripped);
    const QString baseName = info.completeBaseName().isEmpty() ? info.fileName() : info.completeBaseName();
    const QString directory = normalizedPath(info.path());
    if (directory.isEmpty() || directory == QStringLiteral("."))
    {
        return baseName;
    }
    return directory + QLatin1Char('/') + baseName;
}

void assignSlot(ArtifactGroup& group, const QString& role, const QString& path, double bytes)
{
    ArtifactSlot* slot = nullptr;
    if (role == QStringLiteral("before"))
    {
        slot = &group.before;
    }
    else if (role == QStringLiteral("after"))
    {
        slot = &group.after;
    }
    else if (role == QStringLiteral("diff"))
    {
        slot = &group.diff;
    }

    if (slot == nullptr || slot->present)
    {
        return;
    }
    slot->path = normalizedPath(path);
    slot->bytes = bytes;
    slot->present = true;
}

QString groupStatus(const ArtifactGroup& group)
{
    if (group.before.present && group.after.present && group.diff.present)
    {
        return QStringLiteral("paired_with_diff");
    }
    if (group.before.present && group.after.present)
    {
        return QStringLiteral("paired");
    }
    if (!group.before.present && !group.after.present && group.diff.present)
    {
        return QStringLiteral("diff_only");
    }
    return QStringLiteral("incomplete");
}

void insertSlot(QJsonObject& object, QJsonObject& bytes, const QString& role, const ArtifactSlot& slot)
{
    if (!slot.present)
    {
        return;
    }
    object.insert(role, slot.path);
    bytes.insert(role, slot.bytes);
}

QJsonObject groupToJson(const ArtifactGroup& group)
{
    QJsonObject out {
        {QStringLiteral("kind"), group.kind},
        {QStringLiteral("key"), group.key},
        {QStringLiteral("status"), groupStatus(group)},
    };
    QJsonObject bytes;
    insertSlot(out, bytes, QStringLiteral("before"), group.before);
    insertSlot(out, bytes, QStringLiteral("after"), group.after);
    insertSlot(out, bytes, QStringLiteral("diff"), group.diff);
    out.insert(QStringLiteral("bytes"), bytes);
    return out;
}

QJsonObject strategyJson()
{
    return {
        {QStringLiteral("image"),
         QStringLiteral("Index existing before/after/diff image artifacts by normalized relative name; pixel diff generation is handled by external testdiff tools.")},
        {QStringLiteral("property"),
         QStringLiteral("Index existing JSON/XML/YAML/CSV/BREP property artifacts by normalized relative name for later structured comparison.")},
        {QStringLiteral("performance"),
         QStringLiteral("Index existing perf/timing/benchmark artifacts by normalized relative name; metric extraction remains a later parser step.")},
    };
}
} // namespace

QJsonObject TestdiffArtifactIndex::build(const QJsonArray& artifactFiles)
{
    QJsonObject counts = initialCounts();
    QMap<QString, ArtifactGroup> groups;
    const QStringList kinds = supportedKinds();

    for (const QJsonValue& value : artifactFiles)
    {
        const QJsonObject file = value.toObject();
        const QString kind = file.value(QStringLiteral("kind")).toString();
        if (!kinds.contains(kind))
        {
            continue;
        }

        const QString role = file.value(QStringLiteral("role")).toString(QStringLiteral("other"));
        const QString path = file.value(QStringLiteral("path")).toString();
        const double bytes = file.value(QStringLiteral("bytes")).toDouble();
        incrementKindRole(counts, kind, role);

        if (path.isEmpty()
            || (role != QStringLiteral("before")
                && role != QStringLiteral("after")
                && role != QStringLiteral("diff")))
        {
            continue;
        }

        const QString key = artifactKey(path, role);
        const QString groupId = kind + QLatin1Char('\n') + key;
        ArtifactGroup group = groups.value(groupId);
        if (group.kind.isEmpty())
        {
            group.kind = kind;
            group.key = key;
        }
        assignSlot(group, role, path, bytes);
        groups.insert(groupId, group);
    }

    QJsonArray groupsJson;
    for (auto it = groups.cbegin(); it != groups.cend(); ++it)
    {
        groupsJson.append(groupToJson(it.value()));
    }

    return {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("supported_kinds"), supportedKindsJson()},
        {QStringLiteral("counts"), counts},
        {QStringLiteral("groups"), groupsJson},
        {QStringLiteral("strategy"), strategyJson()},
    };
}
} // namespace occtdebug
