#include "workbench/TestgridTablePresenter.h"

#include <QTableWidget>
#include <QTableWidgetItem>

namespace
{
QTableWidgetItem* readonlyItem(const QString& text)
{
    auto* out = new QTableWidgetItem(text);
    out->setFlags(out->flags() & ~Qt::ItemIsEditable);
    return out;
}
} // namespace

namespace occtdebug
{
QVector<QStringList> TestgridTablePresenter::rowsToCells(const QVector<TestgridRow>& rows)
{
    QVector<QStringList> cells;
    cells.reserve(rows.size());
    for (const TestgridRow& row : rows)
    {
        cells.push_back({
            row.module,
            row.runCount,
            row.passCount,
            row.failCount,
            row.passRate,
        });
    }
    return cells;
}

void TestgridTablePresenter::applyToTable(QTableWidget* table, const QVector<TestgridRow>& rows)
{
    if (table == nullptr)
    {
        return;
    }

    table->setRowCount(0);
    const QVector<QStringList> cells = rowsToCells(rows);
    for (const QStringList& rowCells : cells)
    {
        const int row = table->rowCount();
        table->insertRow(row);
        for (int column = 0; column < rowCells.size(); ++column)
        {
            table->setItem(row, column, readonlyItem(rowCells[column]));
        }
    }
}
} // namespace occtdebug
