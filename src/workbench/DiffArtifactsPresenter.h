#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

class QTableWidget;

namespace occtdebug
{
class DiffArtifactsPresenter
{
public:
    struct Filter
    {
        QString kind;
        QString status;
        QString searchText;
    };

    static QVector<QStringList> indexRows(const QJsonObject& testdiffArtifacts, const Filter& filter = {});
    static QVector<QStringList> analysisRows(const QJsonObject& testdiffArtifacts, const Filter& filter = {});
    static void applyIndexToTable(QTableWidget* table, const QJsonObject& testdiffArtifacts, const Filter& filter = {});
    static void applyAnalysisToTable(QTableWidget* table, const QJsonObject& testdiffArtifacts, const Filter& filter = {});
};
} // namespace occtdebug
