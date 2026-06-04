#pragma once

#include "core/case/CaseManifest.h"

#include <QString>
#include <QVector>

namespace occtdebug
{
struct TestdiffEntry
{
    QString name;
    QString status;
    QString metric;
    QString note;
};

struct TestdiffSummary
{
    QVector<TestdiffEntry> entries;
    int changedCount = 0;
    int failedCount = 0;

    QString summaryText() const;
};

class VerificationResultParser
{
public:
    static QVector<TestgridRow> parseTestgridText(const QString& text);
    static TestdiffSummary parseTestdiffText(const QString& text);
};
} // namespace occtdebug
