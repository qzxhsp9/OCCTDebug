#pragma once

#include "core/case/CaseManifest.h"

class QTableWidget;

namespace occtdebug
{
class TestgridTablePresenter
{
public:
    static QVector<QStringList> rowsToCells(const QVector<TestgridRow>& rows);
    static void applyToTable(QTableWidget* table, const QVector<TestgridRow>& rows);
};
} // namespace occtdebug
