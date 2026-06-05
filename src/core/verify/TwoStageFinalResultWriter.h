#pragma once

#include "core/case/CaseManifest.h"
#include "core/verify/VerificationResultParser.h"

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace occtdebug
{
struct TwoStageFinalResultWriterInput
{
    QString caseId;
    QString workspaceRoot;
    QString finalStatus;
    QString note;
    VerificationPlan plan;
    bool patchApplied = false;
    QVector<TestgridRow> afterRows;
    TestdiffSummary testdiff;
    QVector<VerificationFailureDetail> failureDetails;
    VerificationTimingSummary timing;
    QJsonObject testdiffArtifacts;
    TestgridComparison comparison;
    QString beforeSummaryPath;
    QString afterSummaryPath;
};

struct TwoStageFinalResultWriterResult
{
    QJsonObject finalResult;
    QString twoStageResultPath;
    QString legacyResultPath;
};

class TwoStageFinalResultWriter
{
public:
    static bool writeFinalResult(const TwoStageFinalResultWriterInput& input,
                                 TwoStageFinalResultWriterResult* out,
                                 QString* error = nullptr);
};
} // namespace occtdebug
