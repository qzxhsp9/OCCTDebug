#pragma once

#include "core/case/CaseManifest.h"

namespace occtdebug
{
struct WorkbenchMockData
{
    CaseManifest manifest;
    QString caseId;
    QString caseStatus;
    QString occtVersion;
    QString toolchain;
    QString platform;
    QString sourceText;
    QString reproScript;
    QString geometrySummary;
    QString evidenceSummary;
    QString diffSummary;
    QString environmentSummary;
    QString diagnosis;
    int diagnosisConfidence = 0;
    QString patchDiff;
    QString drawConsoleText;
    QString cmakeConsoleText;

    QVector<CaseSummary> cases;
    QVector<WorkflowStep> workflowSteps;
    QVector<LabelValue> keyInputs;
    QVector<GeometryCheck> geometryChecks;
    QVector<EvidenceRecord> evidenceItems;
    QVector<LabelValue> verificationItems;
    QVector<SimilarCase> similarCases;
    QVector<TestgridRow> testgridRows;
};

WorkbenchMockData createWorkbenchDataFromCase(const CaseManifest& manifest);
WorkbenchMockData createMockWorkbenchData();
} // namespace occtdebug
