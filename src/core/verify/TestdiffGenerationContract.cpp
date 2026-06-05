#include "core/verify/TestdiffGenerationContract.h"

#include <QJsonArray>
#include <QString>

namespace occtdebug
{
namespace
{
QJsonObject generatorContract(
    const QString& id,
    const QString& kind,
    const QString& generatedRole,
    const QJsonArray& requiredInputs,
    const QString& outputPattern,
    const QString& sidecarPattern,
    const QJsonArray& sidecarFields)
{
    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("kind"), kind},
        {QStringLiteral("generated_role"), generatedRole},
        {QStringLiteral("required_inputs"), requiredInputs},
        {QStringLiteral("output_pattern"), outputPattern},
        {QStringLiteral("sidecar_pattern"), sidecarPattern},
        {QStringLiteral("sidecar_fields"), sidecarFields},
    };
}
} // namespace

QString TestdiffGenerationContract::outputRoot()
{
    return QStringLiteral("artifacts/testdiff/generated");
}

QString TestdiffGenerationContract::caseManifestField()
{
    return QStringLiteral("verification.testdiff_generation.enabled_generators");
}

QString TestdiffGenerationContract::configManifestField()
{
    return QStringLiteral("verification.testdiff_generation");
}

QString TestdiffGenerationContract::failureReportPath()
{
    return outputRoot() + QStringLiteral("/failure_report.json");
}

QString TestdiffGenerationContract::sidecarSuffix()
{
    return QStringLiteral(".meta.json");
}

QJsonObject TestdiffGenerationContract::defaultConfig()
{
    return {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("enabled_generators"), QJsonArray {}},
        {QStringLiteral("tolerances"), QJsonObject {
             {QStringLiteral("image_pixel_diff"), QJsonObject {
                  {QStringLiteral("pixel_abs"), 0.0},
                  {QStringLiteral("max_changed_ratio"), 0.0},
              }},
             {QStringLiteral("property_structural_diff"), QJsonObject {
                  {QStringLiteral("numeric_abs"), 0.000001},
                  {QStringLiteral("numeric_rel"), 0.000001},
              }},
         }},
        {QStringLiteral("thresholds"), QJsonObject {
             {QStringLiteral("performance_trend_diff"), QJsonObject {
                  {QStringLiteral("regression_percent"), 5.0},
                  {QStringLiteral("min_sample_count"), 1},
              }},
         }},
        {QStringLiteral("failure_report"), QJsonObject {
             {QStringLiteral("path"), failureReportPath()},
             {QStringLiteral("include_sanitized_inputs"), true},
             {QStringLiteral("issue_fields"), QJsonArray {
                  QStringLiteral("generator_id"),
                  QStringLiteral("status"),
                  QStringLiteral("reason"),
                  QStringLiteral("blocked_by"),
                  QStringLiteral("input_artifacts"),
                  QStringLiteral("current_inputs"),
                  QStringLiteral("config"),
              }},
         }},
    };
}

QJsonObject TestdiffGenerationContract::failureReportContract()
{
    return {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("path"), failureReportPath()},
        {QStringLiteral("written_only_when_generation_attempted"), true},
        {QStringLiteral("status_values"), QJsonArray {
             QStringLiteral("blocked"),
             QStringLiteral("failed"),
             QStringLiteral("skipped"),
             QStringLiteral("passed"),
         }},
        {QStringLiteral("issue_fields"), QJsonArray {
             QStringLiteral("generator_id"),
             QStringLiteral("kind"),
             QStringLiteral("status"),
             QStringLiteral("reason"),
             QStringLiteral("blocked_by"),
             QStringLiteral("input_artifacts"),
             QStringLiteral("current_inputs"),
             QStringLiteral("config"),
         }},
        {QStringLiteral("privacy_rules"), QJsonArray {
             QStringLiteral("Failure reports store case-relative artifact paths only."),
             QStringLiteral("Absolute runner output paths must be reduced to sanitized labels."),
             QStringLiteral("Private model filenames must not be exported without explicit user action."),
         }},
    };
}

QJsonObject TestdiffGenerationContract::build()
{
    const QString root = outputRoot();
    QJsonArray generators;
    generators.append(generatorContract(
        QStringLiteral("image_pixel_diff"),
        QStringLiteral("image"),
        QStringLiteral("diff"),
        QJsonArray {QStringLiteral("before image"), QStringLiteral("after image")},
        root + QStringLiteral("/image/{key}.pixel_diff.png"),
        root + QStringLiteral("/image/{key}.pixel_diff.meta.json"),
        QJsonArray {
            QStringLiteral("generator_id"),
            QStringLiteral("artifact"),
            QStringLiteral("artifact_status"),
            QStringLiteral("input_artifacts"),
            QStringLiteral("algorithm"),
            QStringLiteral("tolerance"),
            QStringLiteral("created_at"),
            QStringLiteral("status"),
        }));
    generators.append(generatorContract(
        QStringLiteral("property_structural_diff"),
        QStringLiteral("property"),
        QStringLiteral("diff"),
        QJsonArray {
            QStringLiteral("before property JSON"),
            QStringLiteral("after property JSON"),
            QStringLiteral("schema or key matching policy"),
        },
        root + QStringLiteral("/property/{key}.structural_diff.json"),
        root + QStringLiteral("/property/{key}.structural_diff.meta.json"),
        QJsonArray {
            QStringLiteral("generator_id"),
            QStringLiteral("artifact"),
            QStringLiteral("artifact_status"),
            QStringLiteral("input_artifacts"),
            QStringLiteral("schema_policy"),
            QStringLiteral("status"),
        }));
    generators.append(generatorContract(
        QStringLiteral("performance_trend_diff"),
        QStringLiteral("performance"),
        QStringLiteral("diff"),
        QJsonArray {
            QStringLiteral("current performance metrics"),
            QStringLiteral("baseline or before metrics"),
            QStringLiteral("threshold policy"),
        },
        root + QStringLiteral("/performance/{key}.trend_diff.json"),
        root + QStringLiteral("/performance/{key}.trend_diff.meta.json"),
        QJsonArray {
            QStringLiteral("generator_id"),
            QStringLiteral("artifact"),
            QStringLiteral("artifact_status"),
            QStringLiteral("input_artifacts"),
            QStringLiteral("baseline"),
            QStringLiteral("threshold_policy"),
            QStringLiteral("status"),
        }));

    return {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("mode"), QStringLiteral("opt_in")},
        {QStringLiteral("enabled_by_default"), false},
        {QStringLiteral("config_manifest_field"), configManifestField()},
        {QStringLiteral("case_manifest_field"), caseManifestField()},
        {QStringLiteral("output_root"), root},
        {QStringLiteral("sidecar_suffix"), sidecarSuffix()},
        {QStringLiteral("default_config"), defaultConfig()},
        {QStringLiteral("failure_report"), failureReportContract()},
        {QStringLiteral("generators"), generators},
        {QStringLiteral("privacy_rules"), QJsonArray {
             QStringLiteral("All generated artifact paths are case-relative."),
             QStringLiteral("Sidecars must not store runner output absolute paths."),
             QStringLiteral("Private CAD/model filenames must be sanitized before report export."),
         }},
    };
}
} // namespace occtdebug
