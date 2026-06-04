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
    QString marker;
    QString title;
    QString state;
};

struct GeometryCheck
{
    QString name;
    QString status;
    QString note;
};

struct TestgridRow
{
    QString module;
    QString runCount;
    QString passCount;
    QString failCount;
    QString passRate;
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
    QString geometrySummary;
    QString evidenceSummary;
    QString diffSummary;
    QString environmentSummary;
    QString diagnosis;
    int diagnosisConfidence = 0;
    QString patchDiff;
    QString drawConsoleText;
    QString cmakeConsoleText;

    QVector<CaseSummary> caseList;
    QVector<WorkflowStep> workflowSteps;
    QVector<LabelValue> keyInputs;
    QVector<GeometryCheck> geometryChecks;
    QVector<EvidenceRecord> evidenceItems;
    QVector<LabelValue> verificationItems;
    QVector<SimilarCase> similarCases;
    QVector<TestgridRow> testgridRows;

    static std::optional<CaseManifest> fromJson(const QJsonObject& object, QString* error = nullptr);
    static std::optional<CaseManifest> loadFromFile(const QString& filePath, QString* error = nullptr);
    QJsonObject toJson() const;
};
} // namespace occtdebug
