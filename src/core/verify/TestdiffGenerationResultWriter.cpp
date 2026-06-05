#include "core/verify/TestdiffGenerationResultWriter.h"

#include "core/verify/TestdiffGenerationContract.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonValue>
#include <QSaveFile>

namespace occtdebug
{
namespace
{
bool isCaseRelativePath(const QString& path)
{
    const QString normalized = QDir::fromNativeSeparators(path).trimmed();
    return !normalized.isEmpty()
        && !normalized.startsWith(QLatin1Char('/'))
        && !normalized.startsWith(QLatin1Char('\\'))
        && !normalized.contains(QStringLiteral("://"))
        && !normalized.contains(QLatin1Char(':'))
        && !normalized.split(QLatin1Char('/'), Qt::SkipEmptyParts).contains(QStringLiteral(".."));
}

bool writeJsonFile(const QString& path, const QJsonObject& object, QString* error)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot write %1: %2").arg(path, file.errorString());
        }
        return false;
    }
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    if (!file.commit())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot commit %1: %2").arg(path, file.errorString());
        }
        return false;
    }
    return true;
}

QJsonArray sanitizedStringArray(const QJsonArray& values)
{
    QJsonArray out;
    for (const QJsonValue& value : values)
    {
        const QString path = QDir::fromNativeSeparators(value.toString()).trimmed();
        if (isCaseRelativePath(path))
        {
            out.append(path);
        }
    }
    return out;
}

QString policyFailureReportPath(const QJsonObject& generationPolicy)
{
    const QString path = generationPolicy.value(QStringLiteral("failure_report"))
        .toObject()
        .value(QStringLiteral("path"))
        .toString();
    return path.isEmpty() ? TestdiffGenerationContract::failureReportPath() : path;
}
} // namespace

QString TestdiffGenerationResultWriter::sidecarPathForArtifact(const QString& artifactPath)
{
    const QString normalized = QDir::fromNativeSeparators(artifactPath).trimmed();
    return normalized.isEmpty()
        ? QString()
        : normalized + TestdiffGenerationContract::sidecarSuffix();
}

QJsonObject TestdiffGenerationResultWriter::buildSidecar(const TestdiffGenerationSidecarInput& input)
{
    return {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("generator_id"), input.generatorId},
        {QStringLiteral("kind"), input.kind},
        {QStringLiteral("role"), input.role.isEmpty() ? QStringLiteral("diff") : input.role},
        {QStringLiteral("artifact"), QDir::fromNativeSeparators(input.artifactPath)},
        {QStringLiteral("artifact_status"), input.artifactStatus.isEmpty() ? QStringLiteral("not_generated") : input.artifactStatus},
        {QStringLiteral("input_artifacts"), sanitizedStringArray(input.inputArtifacts)},
        {QStringLiteral("algorithm"), input.algorithm},
        {QStringLiteral("config"), input.config},
        {QStringLiteral("status"), input.status.isEmpty() ? QStringLiteral("blocked") : input.status},
        {QStringLiteral("note"), input.note},
        {QStringLiteral("created_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
    };
}

bool TestdiffGenerationResultWriter::writeSidecar(
    const QString& caseRoot,
    const TestdiffGenerationSidecarInput& input,
    QString* error)
{
    if (!isCaseRelativePath(input.artifactPath))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("generated artifact path must be case-relative: %1").arg(input.artifactPath);
        }
        return false;
    }

    const QString sidecarPath = sidecarPathForArtifact(input.artifactPath);
    if (!isCaseRelativePath(sidecarPath))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("sidecar path must be case-relative: %1").arg(sidecarPath);
        }
        return false;
    }
    return writeJsonFile(QDir(caseRoot).filePath(sidecarPath), buildSidecar(input), error);
}

QJsonObject TestdiffGenerationResultWriter::buildFailureReport(
    const QString& caseId,
    const QJsonObject& generationPolicy,
    const QString& note)
{
    QJsonArray issues;
    for (const QJsonValue& value : generationPolicy.value(QStringLiteral("generators")).toArray())
    {
        const QJsonObject generator = value.toObject();
        if (!generator.value(QStringLiteral("opt_in_requested")).toBool(false))
        {
            continue;
        }
        issues.append(QJsonObject {
            {QStringLiteral("generator_id"), generator.value(QStringLiteral("id")).toString()},
            {QStringLiteral("kind"), generator.value(QStringLiteral("kind")).toString()},
            {QStringLiteral("status"), generator.value(QStringLiteral("candidate")).toBool(false)
                 ? QStringLiteral("blocked")
                 : QStringLiteral("skipped")},
            {QStringLiteral("reason"), generator.value(QStringLiteral("reason")).toString()},
            {QStringLiteral("blocked_by"), generator.value(QStringLiteral("blocked_by")).toArray()},
            {QStringLiteral("input_artifacts"), sanitizedStringArray(generator.value(QStringLiteral("input_artifacts")).toArray())},
            {QStringLiteral("current_inputs"), generator.value(QStringLiteral("current_inputs")).toObject()},
            {QStringLiteral("config"), generator.value(QStringLiteral("effective_config")).toObject()},
        });
    }

    const QString status = issues.isEmpty()
        ? QStringLiteral("skipped")
        : QStringLiteral("blocked");
    return {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("case_id"), caseId},
        {QStringLiteral("created_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {QStringLiteral("status"), status},
        {QStringLiteral("path"), policyFailureReportPath(generationPolicy)},
        {QStringLiteral("policy"), generationPolicy.value(QStringLiteral("policy")).toString()},
        {QStringLiteral("generation_performed"), generationPolicy.value(QStringLiteral("generation_performed")).toBool(false)},
        {QStringLiteral("note"), note},
        {QStringLiteral("issues"), issues},
        {QStringLiteral("privacy"), QJsonObject {
             {QStringLiteral("case_relative_paths_only"), true},
             {QStringLiteral("absolute_runner_paths_stored"), false},
         }},
    };
}

bool TestdiffGenerationResultWriter::writeFailureReport(
    const QString& caseRoot,
    const QString& caseId,
    const QJsonObject& generationPolicy,
    const QString& relativePath,
    QString* error)
{
    const QString path = relativePath.trimmed().isEmpty()
        ? policyFailureReportPath(generationPolicy)
        : QDir::fromNativeSeparators(relativePath).trimmed();
    if (!isCaseRelativePath(path))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("failure report path must be case-relative: %1").arg(path);
        }
        return false;
    }

    QJsonObject report = buildFailureReport(caseId, generationPolicy);
    report.insert(QStringLiteral("path"), path);
    return writeJsonFile(QDir(caseRoot).filePath(path), report, error);
}
} // namespace occtdebug
