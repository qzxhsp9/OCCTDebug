#include "core/verify/TestdiffGenerationPolicy.h"

#include "core/verify/TestdiffGenerationContract.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

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
        {QStringLiteral("candidate"), candidate},
        {QStringLiteral("generation_performed"), false},
        {QStringLiteral("reason"), reason},
        {QStringLiteral("required_inputs"), requiredInputs},
        {QStringLiteral("current_inputs"), currentInputs},
        {QStringLiteral("blocked_by"), blockedBy},
    };
}
} // namespace

QJsonObject TestdiffGenerationPolicy::build(const QJsonObject& artifactIndex, const QJsonObject& artifactAnalysis)
{
    const KindState image = scanKindState(artifactIndex, artifactAnalysis, QStringLiteral("image"));
    const KindState property = scanKindState(artifactIndex, artifactAnalysis, QStringLiteral("property"));
    const KindState performance = scanKindState(artifactIndex, artifactAnalysis, QStringLiteral("performance"));

    QJsonArray generators;
    generators.append(disabledGenerator(
        QStringLiteral("image_pixel_diff"),
        QStringLiteral("image"),
        image.pairedGroups > 0,
        QStringLiteral("Pixel diff generation is intentionally disabled until an image decoder and comparison contract are defined."),
        QJsonArray {QStringLiteral("before image"), QStringLiteral("after image")},
        QJsonArray {QStringLiteral("no bundled image decoder"), QStringLiteral("no pixel comparator tolerance policy"), QStringLiteral("no generated image artifact contract")},
        image));

    generators.append(disabledGenerator(
        QStringLiteral("property_structural_diff"),
        QStringLiteral("property"),
        property.pairedGroups > 0 && property.parsedPropertyGroups > 0,
        QStringLiteral("Structural property diff generation is intentionally disabled until before/after schema matching rules are defined."),
        QJsonArray {QStringLiteral("before property JSON"), QStringLiteral("after property JSON"), QStringLiteral("schema or key matching policy")},
        QJsonArray {QStringLiteral("no structural diff algorithm"), QStringLiteral("no schema compatibility contract"), QStringLiteral("no conflict reporting format")},
        property,
        QJsonObject {{QStringLiteral("parsed_property_groups"), property.parsedPropertyGroups}}));

    generators.append(disabledGenerator(
        QStringLiteral("performance_trend_diff"),
        QStringLiteral("performance"),
        performance.performanceMetrics > 0,
        QStringLiteral("Performance trend generation is intentionally disabled until a baseline/history model and threshold policy are defined."),
        QJsonArray {QStringLiteral("current performance metrics"), QStringLiteral("baseline or before metrics"), QStringLiteral("threshold policy")},
        QJsonArray {QStringLiteral("no baseline store"), QStringLiteral("no trend threshold policy"), QStringLiteral("no statistical comparison contract")},
        performance,
        QJsonObject {{QStringLiteral("parsed_metric_count"), performance.performanceMetrics}}));

    return {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("policy"), QStringLiteral("boundary_only")},
        {QStringLiteral("generation_performed"), false},
        {QStringLiteral("contract"), TestdiffGenerationContract::build()},
        {QStringLiteral("generators"), generators},
        {QStringLiteral("next_steps"), QJsonArray {
             QStringLiteral("Define algorithm-specific tolerances and failure reporting."),
             QStringLiteral("Add opt-in generators only after smoke tests cover generated outputs."),
             QStringLiteral("Keep generated artifact paths case-relative and sidecar metadata sanitized."),
         }},
    };
}
} // namespace occtdebug
