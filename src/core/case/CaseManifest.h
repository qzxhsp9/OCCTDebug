#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

#include <optional>

namespace occtdebug
{
struct LabelValue
{
    QString label;
    QString value;
};

struct CaseSummary
{
    QString id;
    QString status;
    QString title;
    QString createdAt;
};

struct WorkflowStep
{
    QString id;
    QString marker;
    QString title;
    QString state;
    QString note;
};

struct WorkflowState
{
    QString activeStepId;
    QVector<WorkflowStep> steps;
};

struct WorkspaceLayout
{
    QString activeCenterTab;
    QString activeBottomTab;
    int leftWidth = 300;
    int centerWidth = 980;
    int rightWidth = 380;
    int bottomHeight = 235;
};

struct GeometryCheck
{
    QString name;
    QString status;
    QString note;
};

struct InputFileRecord
{
    QString path;
    QString originalName;
    QString sha256;
    qint64 bytes = 0;
    QString importedAt;
};

struct TestgridRow
{
    QString module;
    QString runCount;
    QString passCount;
    QString failCount;
    QString passRate;
};

struct VerificationPlan
{
    QString testgridRoot;
    QString testgridExecutable;
    QString testgridArguments;
    QString testgridGroup;
    QString testgridGrid;
    QString testgridCase;
    QString testdiffExecutable;
    QString testdiffArguments;
    QString testdiffOutputRoot;
};

struct TestdiffGenerationConfig
{
    QVector<QString> enabledGenerators;
    double imagePixelTolerance = 0.0;
    double propertyNumericTolerance = 0.000001;
    double performanceRegressionPercent = 5.0;
    QString failureReportPath;
};

struct ReproStatus
{
    QString overall;
    QString draw;
    QString cpp;
    QString testgrid;
    QString updatedAt;
    QString summary;
};

struct SimilarCase
{
    QString id;
    QString title;
    QString score;
};

struct EvidenceRecord
{
    QString type;
    QString title;
    QString summary;
    QString link;
    QString sourceFile;
    int sourceLine = 0;
    QString logFile;
    int logLine = 0;
    QString stackFrame;
    QString geometryObject;
};

struct CaseManifest
{
    QString caseId;
    QString title;
    QString status;
    QString createdAt;
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

    QVector<CaseSummary> caseList;
    QVector<WorkflowStep> workflowSteps;
    WorkflowState workflowState;
    WorkspaceLayout workspaceLayout;
    QVector<LabelValue> keyInputs;
    QVector<InputFileRecord> inputFiles;
    QVector<GeometryCheck> geometryChecks;
    QVector<EvidenceRecord> evidenceItems;
    QVector<LabelValue> verificationItems;
    VerificationPlan verificationPlan;
    TestdiffGenerationConfig testdiffGenerationConfig;
    QVector<SimilarCase> similarCases;
    QVector<TestgridRow> testgridRows;
    QVector<LabelValue> patchReviewItems;

    static std::optional<CaseManifest> fromJson(const QJsonObject& object, QString* error = nullptr);
    static std::optional<CaseManifest> loadFromFile(const QString& filePath, QString* error = nullptr);
    bool saveToFile(const QString& filePath, QString* error = nullptr) const;
    QJsonObject toJson() const;
};
} // namespace occtdebug
