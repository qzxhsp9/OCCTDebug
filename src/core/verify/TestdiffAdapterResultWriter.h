#pragma once

#include "core/case/CaseManifest.h"
#include "core/runner/CommandRunner.h"
#include "core/verify/TestdiffRunnerAdapter.h"
#include "core/verify/VerificationResultParser.h"

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace occtdebug
{
struct TestdiffAdapterResultWriterInput
{
    QString workspaceRoot;
    VerificationPlan plan;
    QString outputRoot;
    QVector<TestgridRow> existingRows;
    CommandResult commandResult;
    QString note;
};

struct TestdiffAdapterResultWriterResult
{
    QString status;
    QString adapterStatus;
    QString diffSummary;
    TestdiffSummary testdiff;
    TestdiffRunnerImportResult importResult;
    QJsonObject adapterResult;
    QJsonObject testgridResult;
    QString adapterResultPath;
    QString adapterManifestPath;
    QString testgridResultPath;
};

class TestdiffAdapterResultWriter
{
public:
    static bool writeResult(const TestdiffAdapterResultWriterInput& input,
                            TestdiffAdapterResultWriterResult* result,
                            QString* error = nullptr);
};
} // namespace occtdebug
