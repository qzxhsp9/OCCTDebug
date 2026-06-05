#include "core/verify/TestdiffGenerationPolicy.h"

#include "core/verify/TestdiffGenerationContract.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QStringList>

namespace occtdebug
{
namespace
{
struct KindState
{
    int pairedGroups = 0;
    int runnerDiffGroups = 0;
    int parsedPropertyGroups = 0;
    int performanceMetrics = 0;
};

KindState scanKindState(const QJsonObject& artifactIndex, const QJsonObject& artifactAnalysis, const QString& kind)
{
    KindState state;
    for (const QJsonValue& value : artifactIndex.value(QStringLiteral("groups")).toArray())
    {
        const QJsonObject group = value.toObject();
        if (group.value(QStringLiteral("kind")).toString() != kind)
        {
            continue;
        }
        if (group.contains(QStringLiteral("before")) && group.contains(QStringLiteral("after")))
        {
            ++state.pairedGroups;
        }
        if (group.contains(QStringLiteral("diff")))
        {
            ++state.runnerDiffGroups;
        }
    }

    const QJsonObject summary = artifactAnalysis.value(QStringLiteral("summary")).toObject();
    if (kind == QStringLiteral("property"))
    {
        state.parsedPropertyGroups = summary.value(QStringLiteral("property_json_parsed")).toInt();
    }
    if (kind == QStringLiteral("performance"))
    {
        state.performanceMetrics = summary.value(QStringLiteral("performance_metrics")).toInt();
    }
    return state;
}

QJsonObject disabledGenerator(
    const QString& id,
    const QString& kind,
    bool candidate,
    const QString& reason,
    const QJsonArray& requiredInputs,
    const QJsonArray& blockedBy,
    const KindState& state,
    bool optInRequested,
    const QJsonObject& effectiveConfig,
    const QJsonObject& failureReport,
    QJsonObject extraInputs = {})
{
    QJsonObject currentInputs {
        {QStringLiteral("paired_groups"), state.pairedGroups},
        {QStringLiteral("runner_diff_groups"), state.runnerDiffGroups},
    };
    for (auto it = extraInputs.constBegin(); it != extraInputs.constEnd(); ++it)
    {
        currentInputs.insert(it.key(), it.value());
    }

    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("kind"), kind},
        {QStringLiteral("enabled"), false},
        {QStringLiteral("opt_in_requested"), optInRequested},
        {QStringLiteral("candidate"), candidate},
        {QStringLiteral("generation_performed"), false},
        {QStringLiteral("reason"), reason},
        {QStringLiteral("required_inputs"), requiredInputs},
        {QStringLiteral("current_inputs"), currentInputs},
        {QStringLiteral("effective_config"), effectiveConfig},
        {QStringLiteral("failure_report"), failureReport},
        {QStringLiteral("blocked_by"), blockedBy},
    };
}

QStringList enabledGeneratorIds(const QJsonObject& generationConfig)
{
    QStringList ids;
    for (const QJsonValue& value : generationConfig.value(QStringLiteral("enabled_generators")).toArray())
    {
        const QString id = value.toString();
        if (!id.isEmpty() && !ids.contains(id))
        {
            ids.push_back(id);
        }
    }
    return ids;
}

QJsonObject objectOrFallback(const QJsonObject& parent, const QString& key, const QJsonObject& fallback)
{
    const QJsonValue value = parent.value(key);
    return value.isObject() ? value.toObject() : fallback;
}

QJsonObject generatorConfig(const QJsonObject& generationConfig, const QString& id)
{
    const QJsonObject defaults = TestdiffGenerationContract::defaultConfig();
    const QJsonObject toleranceDefaults = defaults.value(QStringLiteral("tolerances")).toObject();
    const QJsonObject thresholdDefaults = defaults.value(QStringLiteral("thresholds")).toObject();
    const QJsonObject tolerances = objectOrFallback(
        generationConfig,
        QStringLiteral("tolerances"),
        toleranceDefaults);
    const QJsonObject thresholds = objectOrFallback(
        generationConfig,
        QStringLiteral("thresholds"),
        thresholdDefaults);

    QJsonObject effective {
        {QStringLiteral("tolerances"), objectOrFallback(
             tolerances,
             id,
             toleranceDefaults.value(id).toObject())},
        {QStringLiteral("thresholds"), objectOrFallback(
             thresholds,
             id,
             thresholdDefaults.value(id).toObject())},
    };
    return effective;
}

QString configuredFailureReportPath(const QJsonObject& generationConfig)
{
    const QString configuredPath = generationConfig.value(QStringLiteral("failure_report"))
        .toObject()
        .value(QStringLiteral("path"))
        .toString();
    return configuredPath.isEmpty() ? TestdiffGenerationContract::failureReportPath() : configuredPath;
}

QJsonObject failureReportStatus(const QJsonObject& generationConfig, bool optInRequested, bool candidate)
{
    return {
        {QStringLiteral("path"), configuredFailureReportPath(generationConfig)},
        {QStringLiteral("status"), QStringLiteral("not_written")},
        {QStringLiteral("reason"),
         optInRequested
             ? (candidate
                    ? QStringLiteral("Generator implementation is not enabled yet; N73 records the failure report contract only.")
                    : QStringLiteral("Generator was requested but required before/after inputs are incomplete."))
             : QStringLiteral("Generator was not requested by case opt-in config.")},
    };
}

QJsonArray blockedReasons(
    const QJsonArray& baseReasons,
    bool optInRequested,
    bool candidate)
{
    QJsonArray reasons = baseReasons;
    if (optInRequested)
    {
        reasons.append(QStringLiteral("generator implementation unavailable"));
        if (!candidate)
        {
            reasons.append(QStringLiteral("required paired inputs unavailable"));
        }
    }
    return reasons;
}
} // namespace

QJsonObject TestdiffGenerationPolicy::build(const QJsonObject& artifactIndex, const QJsonObject& artifactAnalysis)
{
    return build(artifactIndex, artifactAnalysis, TestdiffGenerationContract::defaultConfig());
}

QJsonObject TestdiffGenerationPolicy::build(
    const QJsonObject& artifactIndex,
    const QJsonObject& artifactAnalysis,
    const QJsonObject& generationConfig)
{
    const KindState image = scanKindState(artifactIndex, artifactAnalysis, QStringLiteral("image"));
    const KindState property = scanKindState(artifactIndex, artifactAnalysis, QStringLiteral("property"));
    const KindState performance = scanKindState(artifactIndex, artifactAnalysis, QStringLiteral("performance"));
    const QStringList enabledIds = enabledGeneratorIds(generationConfig);

    QJsonArray generators;
    const bool imageRequested = enabledIds.contains(QStringLiteral("image_pixel_diff"));
    const bool imageCandidate = image.pairedGroups > 0;
    generators.append(disabledGenerator(
        QStringLiteral("image_pixel_diff"),
        QStringLiteral("image"),
        imageCandidate,
        imageRequested
            ? QStringLiteral("Pixel diff generation was requested, but the real generator is still disabled until the implementation smoke exists.")
            : QStringLiteral("Pixel diff generation is intentionally disabled until an image decoder and comparison contract are defined."),
        QJsonArray {QStringLiteral("before image"), QStringLiteral("after image")},
        blockedReasons(
            QJsonArray {QStringLiteral("no bundled image decoder"), QStringLiteral("no pixel comparator implementation")},
            imageRequested,
            imageCandidate),
        image,
        imageRequested,
        generatorConfig(generationConfig, QStringLiteral("image_pixel_diff")),
        failureReportStatus(generationConfig, imageRequested, imageCandidate)));

    const bool propertyRequested = enabledIds.contains(QStringLiteral("property_structural_diff"));
    const bool propertyCandidate = property.pairedGroups > 0 && property.parsedPropertyGroups > 0;
    generators.append(disabledGenerator(
        QStringLiteral("property_structural_diff"),
        QStringLiteral("property"),
        propertyCandidate,
        propertyRequested
            ? QStringLiteral("Structural property diff generation was requested, but the real generator is still disabled until failure-report smoke is extended to generated outputs.")
            : QStringLiteral("Structural property diff generation is intentionally disabled until before/after schema matching rules are defined."),
        QJsonArray {QStringLiteral("before property JSON"), QStringLiteral("after property JSON"), QStringLiteral("schema or key matching policy")},
        blockedReasons(
            QJsonArray {QStringLiteral("no structural diff algorithm"), QStringLiteral("no schema compatibility implementation")},
            propertyRequested,
            propertyCandidate),
        property,
        propertyRequested,
        generatorConfig(generationConfig, QStringLiteral("property_structural_diff")),
        failureReportStatus(generationConfig, propertyRequested, propertyCandidate),
        QJsonObject {{QStringLiteral("parsed_property_groups"), property.parsedPropertyGroups}}));

    const bool performanceRequested = enabledIds.contains(QStringLiteral("performance_trend_diff"));
    const bool performanceCandidate = performance.performanceMetrics > 0;
    generators.append(disabledGenerator(
        QStringLiteral("performance_trend_diff"),
        QStringLiteral("performance"),
        performanceCandidate,
        performanceRequested
            ? QStringLiteral("Performance trend generation was requested, but the real generator is still disabled until baseline semantics are implemented.")
            : QStringLiteral("Performance trend generation is intentionally disabled until a baseline/history model and threshold policy are defined."),
        QJsonArray {QStringLiteral("current performance metrics"), QStringLiteral("baseline or before metrics"), QStringLiteral("threshold policy")},
        blockedReasons(
            QJsonArray {QStringLiteral("no baseline store"), QStringLiteral("no statistical comparison implementation")},
            performanceRequested,
            performanceCandidate),
        performance,
        performanceRequested,
        generatorConfig(generationConfig, QStringLiteral("performance_trend_diff")),
        failureReportStatus(generationConfig, performanceRequested, performanceCandidate),
        QJsonObject {{QStringLiteral("parsed_metric_count"), performance.performanceMetrics}}));

    return {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("policy"), QStringLiteral("boundary_only")},
        {QStringLiteral("generation_performed"), false},
        {QStringLiteral("opt_in_requested"), !enabledIds.isEmpty()},
        {QStringLiteral("effective_config"), generationConfig},
        {QStringLiteral("contract"), TestdiffGenerationContract::build()},
        {QStringLiteral("failure_report"), TestdiffGenerationContract::failureReportContract()},
        {QStringLiteral("generators"), generators},
        {QStringLiteral("next_steps"), QJsonArray {
             QStringLiteral("Define algorithm-specific tolerances and failure reporting."),
             QStringLiteral("Add opt-in generators only after smoke tests cover generated outputs."),
             QStringLiteral("Keep generated artifact paths case-relative and sidecar metadata sanitized."),
         }},
    };
}
} // namespace occtdebug
