#pragma once

#include "core/case/CaseManifest.h"

namespace occtdebug
{
struct WorkbenchMockData
{
    CaseManifest manifest;
    QString workspaceRoot;
    QString caseId;
    QString caseStatus;
    QString occtVersion;
    QString toolchain;
    QString platform;
    QString sourceText;
    QString reproScript;
    ReproStatus reproStatus;
    QString geometrySummary;
    QString evidenceSummary;
    QString diffSummary;
    QString environmentSummary;
    QString diagnosis;
    int diagnosisConfidence = 0;
    QString patchDiff;
    QString patchReviewStatus;
    QString patchWorktreeRoot;
    QString patchApplyStatus;
    QString patchApplyLog;
    QString patchSignoffStatus;
    QString patchSignoffNote;
    QString drawConsoleText;
    QString cmakeConsoleText;

    QVector<CaseSummary> cases;
    QVector<WorkflowStep> workflowSteps;
    QVector<LabelValue> keyInputs;
    QVector<GeometryCheck> geometryChecks;
    QVector<EvidenceRecord> evidenceItems;
    QVector<LabelValue> verificationItems;
    VerificationPlan verificationPlan;
    QVector<SimilarCase> similarCases;
    QVector<TestgridRow> testgridRows;
    QVector<LabelValue> patchReviewItems;
};

WorkbenchMockData createWorkbenchDataFromCase(const CaseManifest& manifest);
WorkbenchMockData createWorkbenchDataFromCaseDirectory(const QString& caseDirectory, QString* error = nullptr);
WorkbenchMockData createMockWorkbenchData();
} // namespace occtdebug
