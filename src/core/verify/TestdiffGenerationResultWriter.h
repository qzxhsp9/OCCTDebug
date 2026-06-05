#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

namespace occtdebug
{
struct TestdiffGenerationSidecarInput
{
    QString generatorId;
    QString kind;
    QString role;
    QString artifactPath;
    QString artifactStatus;
    QJsonArray inputArtifacts;
    QJsonObject config;
    QString algorithm;
    QString status;
    QString note;
};

class TestdiffGenerationResultWriter
{
public:
    static QString sidecarPathForArtifact(const QString& artifactPath);
    static QJsonObject buildSidecar(const TestdiffGenerationSidecarInput& input);
    static bool writeSidecar(
        const QString& caseRoot,
        const TestdiffGenerationSidecarInput& input,
        QString* error = nullptr);

    static QJsonObject buildFailureReport(
        const QString& caseId,
        const QJsonObject& generationPolicy,
        const QString& note = QString());
    static bool writeFailureReport(
        const QString& caseRoot,
        const QString& caseId,
        const QJsonObject& generationPolicy,
        const QString& relativePath,
        QString* error = nullptr);
};
} // namespace occtdebug
