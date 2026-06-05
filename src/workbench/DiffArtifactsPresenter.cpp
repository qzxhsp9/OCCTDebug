#include "workbench/DiffArtifactsPresenter.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QTableWidget>
#include <QTableWidgetItem>

namespace
{
bool matchesSearchText(const QStringList& values, const QString& searchText)
{
    const QString needle = searchText.trimmed();
    if (needle.isEmpty())
    {
        return true;
    }
    for (const QString& value : values)
    {
        if (value.contains(needle, Qt::CaseInsensitive))
        {
            return true;
        }
    }
    return false;
}

bool acceptsFilter(const QJsonObject& group,
                   const occtdebug::DiffArtifactsPresenter::Filter& filter,
                   const QStringList& searchValues = {})
{
    const QString kind = filter.kind.trimmed();
    const QString status = filter.status.trimmed();
    return (kind.isEmpty() || group.value(QStringLiteral("kind")).toString() == kind)
        && (status.isEmpty() || group.value(QStringLiteral("status")).toString() == status)
        && matchesSearchText(searchValues, filter.searchText);
}

QString textValue(const QJsonObject& object, const QString& key)
{
    const QJsonValue value = object.value(key);
    if (value.isString())
    {
        return value.toString();
    }
    if (value.isDouble())
    {
        return QString::number(value.toDouble());
    }
    if (value.isBool())
    {
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    }
    return QString();
}

QString rolePathSummary(const QJsonObject& group, const QString& role)
{
    const QJsonValue value = group.value(role);
    if (value.isString())
    {
        return value.toString();
    }
    if (value.isObject())
    {
        return textValue(value.toObject(), QStringLiteral("path"));
    }
    return QStringLiteral("-");
}

QStringList firstKeys(const QJsonArray& keys, int limit)
{
    QStringList out;
    for (int i = 0; i < keys.size() && out.size() < limit; ++i)
    {
        const QString key = keys.at(i).toString();
        if (!key.isEmpty())
        {
            out.push_back(key);
        }
    }
    return out;
}

QString analysisSummary(const QJsonObject& group)
{
    const QString kind = group.value(QStringLiteral("kind")).toString();
    const QJsonObject analysis = group.value(QStringLiteral("analysis")).toObject();
    if (kind == QStringLiteral("image"))
    {
        const QString before = analysis.contains(QStringLiteral("before_supplied"))
            ? textValue(analysis, QStringLiteral("before_supplied"))
            : textValue(analysis, QStringLiteral("has_before"));
        const QString after = analysis.contains(QStringLiteral("after_supplied"))
            ? textValue(analysis, QStringLiteral("after_supplied"))
            : textValue(analysis, QStringLiteral("has_after"));
        return QStringLiteral("diff_supplied=%1 before=%2 after=%3")
            .arg(textValue(analysis, QStringLiteral("diff_supplied_by_runner")),
                 before,
                 after);
    }
    if (kind == QStringLiteral("property"))
    {
        const QJsonObject json = analysis.contains(QStringLiteral("json"))
            ? analysis.value(QStringLiteral("json")).toObject()
            : analysis.value(QStringLiteral("json_summary")).toObject();
        const QString keys = firstKeys(json.value(QStringLiteral("top_level_keys")).toArray(), 5).join(QStringLiteral(", "));
        return QStringLiteral("%1 keys=%2%3")
            .arg(textValue(json, QStringLiteral("json_type")),
                 textValue(json, QStringLiteral("top_level_key_count")),
                 keys.isEmpty() ? QString() : QStringLiteral(" [%1]").arg(keys));
    }
    if (kind == QStringLiteral("performance"))
    {
        const QJsonArray metrics = analysis.value(QStringLiteral("metrics")).toArray();
        QStringList samples;
        for (int i = 0; i < metrics.size() && samples.size() < 3; ++i)
        {
            const QJsonObject metric = metrics.at(i).toObject();
            const QString name = textValue(metric, QStringLiteral("name"));
            const QString value = textValue(metric, QStringLiteral("value"));
            const QString unit = textValue(metric, QStringLiteral("unit"));
            samples.push_back(QStringLiteral("%1=%2%3")
                                  .arg(name.isEmpty() ? QStringLiteral("value") : name,
                                       value,
                                       unit.isEmpty() ? QString() : QStringLiteral(" %1").arg(unit)));
        }
        return QStringLiteral("metrics=%1%2")
            .arg(metrics.size())
            .arg(samples.isEmpty() ? QString() : QStringLiteral(" [%1]").arg(samples.join(QStringLiteral("; "))));
    }
    return QStringLiteral("-");
}

QTableWidgetItem* readonlyItem(const QString& text)
{
    auto* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

void applyRows(QTableWidget* table, const QVector<QStringList>& rows)
{
    if (table == nullptr)
    {
        return;
    }
    table->setRowCount(0);
    for (const QStringList& cells : rows)
    {
        const int row = table->rowCount();
        table->insertRow(row);
        for (int column = 0; column < cells.size(); ++column)
        {
            table->setItem(row, column, readonlyItem(cells.at(column)));
        }
    }
}
} // namespace

namespace occtdebug
{
QVector<QStringList> DiffArtifactsPresenter::indexRows(const QJsonObject& testdiffArtifacts, const Filter& filter)
{
    QVector<QStringList> rows;
    const QJsonArray groups =
        testdiffArtifacts.value(QStringLiteral("artifact_index")).toObject().value(QStringLiteral("groups")).toArray();
    rows.reserve(groups.size());
    for (const QJsonValue& value : groups)
    {
        const QJsonObject group = value.toObject();
        const QString before = rolePathSummary(group, QStringLiteral("before"));
        const QString after = rolePathSummary(group, QStringLiteral("after"));
        const QString diff = rolePathSummary(group, QStringLiteral("diff"));
        if (!acceptsFilter(group,
                filter,
                {
                    group.value(QStringLiteral("kind")).toString(),
                    group.value(QStringLiteral("key")).toString(),
                    group.value(QStringLiteral("status")).toString(),
                    before,
                    after,
                    diff,
                }))
        {
            continue;
        }
        rows.push_back({
            group.value(QStringLiteral("kind")).toString(),
            group.value(QStringLiteral("key")).toString(),
            group.value(QStringLiteral("status")).toString(),
            before,
            after,
            diff,
        });
    }
    return rows;
}

QVector<QStringList> DiffArtifactsPresenter::analysisRows(const QJsonObject& testdiffArtifacts, const Filter& filter)
{
    QVector<QStringList> rows;
    const QJsonArray groups =
        testdiffArtifacts.value(QStringLiteral("artifact_analysis")).toObject().value(QStringLiteral("groups")).toArray();
    rows.reserve(groups.size());
    for (const QJsonValue& value : groups)
    {
        const QJsonObject group = value.toObject();
        const QString summary = analysisSummary(group);
        if (!acceptsFilter(group,
                filter,
                {
                    group.value(QStringLiteral("kind")).toString(),
                    group.value(QStringLiteral("key")).toString(),
                    group.value(QStringLiteral("status")).toString(),
                    summary,
                }))
        {
            continue;
        }
        rows.push_back({
            group.value(QStringLiteral("kind")).toString(),
            group.value(QStringLiteral("key")).toString(),
            group.value(QStringLiteral("status")).toString(),
            summary,
        });
    }
    return rows;
}

void DiffArtifactsPresenter::applyIndexToTable(QTableWidget* table, const QJsonObject& testdiffArtifacts, const Filter& filter)
{
    applyRows(table, indexRows(testdiffArtifacts, filter));
}

void DiffArtifactsPresenter::applyAnalysisToTable(QTableWidget* table, const QJsonObject& testdiffArtifacts, const Filter& filter)
{
    applyRows(table, analysisRows(testdiffArtifacts, filter));
}
} // namespace occtdebug
