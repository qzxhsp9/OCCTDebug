#pragma once

#include "core/case/CaseManifest.h"

#include <QString>
#include <QtGlobal>
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

struct TestgridComparisonRow
{
    QString module;
    QString beforeRunCount;
    QString beforePassCount;
    QString beforeFailCount;
    QString afterRunCount;
    QString afterPassCount;
    QString afterFailCount;
    int passDelta = 0;
    int failDelta = 0;
    QString status;
};

struct TestgridComparison
{
    QVector<TestgridComparisonRow> rows;
    int beforeRunTotal = 0;
    int beforePassTotal = 0;
    int beforeFailTotal = 0;
    int afterRunTotal = 0;
    int afterPassTotal = 0;
    int afterFailTotal = 0;
    int passDelta = 0;
    int failDelta = 0;

    bool isAvailable() const;
    bool hasRegression() const;
    QString summaryText() const;
};

struct VerificationFailureDetail
{
    QString type;
    QString name;
    QString status;
    QString summary;
    QString artifact;
};

struct VerificationTimingEntry
{
    QString name;
    qint64 elapsedMs = 0;
    QString status;
};

struct VerificationTimingSummary
{
    QVector<VerificationTimingEntry> entries;
    qint64 totalElapsedMs = 0;

    QString summaryText() const;
};

class VerificationResultParser
{
public:
    static QVector<TestgridRow> parseTestgridText(const QString& text);
    static TestdiffSummary parseTestdiffText(const QString& text);
    static TestgridComparison compareTestgridRows(const QVector<TestgridRow>& beforeRows,
                                                  const QVector<TestgridRow>& afterRows);
    static QVector<VerificationFailureDetail> failureDetailsForTestgridRows(const QVector<TestgridRow>& rows,
                                                                            const QString& artifact = {});
    static QVector<VerificationFailureDetail> failureDetailsForTestdiff(const TestdiffSummary& summary,
                                                                        const QString& artifact = {});
    static QVector<VerificationFailureDetail> failureDetailsForComparison(const TestgridComparison& comparison,
                                                                          const QString& artifact = {});
    static VerificationTimingSummary timingSummary(const QVector<VerificationTimingEntry>& entries);
};
} // namespace occtdebug
