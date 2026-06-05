#include "core/verify/TestdiffGenerationPolicy.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QTextStream>

namespace
{
bool expect(bool condition, const char* message)
{
    if (!condition)
    {
        QTextStream(stderr) << message << "\n";
    }
    return condition;
}

QJsonObject findGenerator(const QJsonArray& generators, const QString& id)
{
    for (const QJsonValue& value : generators)
    {
        const QJsonObject generator = value.toObject();
        if (generator.value(QStringLiteral("id")).toString() == id)
        {
            return generator;
        }
    }
    return {};
}

bool isCaseRelativePath(const QString& value)
{
    return !value.isEmpty()
        && !value.startsWith(QLatin1Char('/'))
        && !value.startsWith(QLatin1Char('\\'))
        && !value.contains(QStringLiteral("://"))
        && !value.contains(QLatin1Char(':'))
        && !value.contains(QStringLiteral(".."));
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    const QJsonObject artifactIndex {
        {QStringLiteral("groups"), QJsonArray {
             QJsonObject {
                 {QStringLiteral("kind"), QStringLiteral("image")},
                 {QStringLiteral("key"), QStringLiteral("view")},
                 {QStringLiteral("before"), QStringLiteral("artifacts/testdiff/before/view.png")},
                 {QStringLiteral("after"), QStringLiteral("artifacts/testdiff/after/view.png")},
             },
             QJsonObject {
                 {QStringLiteral("kind"), QStringLiteral("performance")},
                 {QStringLiteral("key"), QStringLiteral("timing")},
                 {QStringLiteral("diff"), QStringLiteral("artifacts/testdiff/diff/timing.txt")},
             },
         }},
    };
    const QJsonObject artifactAnalysis {
        {QStringLiteral("summary"), QJsonObject {
             {QStringLiteral("performance_metrics"), 2},
         }},
    };
    const QJsonObject generationConfig {
        {QStringLiteral("enabled_generators"), QJsonArray {
             QStringLiteral("image_pixel_diff"),
             QStringLiteral("performance_trend_diff"),
         }},
        {QStringLiteral("tolerances"), QJsonObject {
             {QStringLiteral("image_pixel_diff"), QJsonObject {
                  {QStringLiteral("pixel_abs"), 2.0},
                  {QStringLiteral("max_changed_ratio"), 0.01},
              }},
         }},
        {QStringLiteral("thresholds"), QJsonObject {
             {QStringLiteral("performance_trend_diff"), QJsonObject {
                  {QStringLiteral("regression_percent"), 7.5},
                  {QStringLiteral("min_sample_count"), 3},
              }},
         }},
        {QStringLiteral("failure_report"), QJsonObject {
             {QStringLiteral("path"), QStringLiteral("artifacts/testdiff/generated/failure_report.json")},
         }},
    };

    const QJsonObject policy = occtdebug::TestdiffGenerationPolicy::build(
        artifactIndex,
        artifactAnalysis,
        generationConfig);
    const QJsonObject topFailureReport = policy.value(QStringLiteral("failure_report")).toObject();
    const QJsonArray generators = policy.value(QStringLiteral("generators")).toArray();
    const QJsonObject image = findGenerator(generators, QStringLiteral("image_pixel_diff"));
    const QJsonObject performance = findGenerator(generators, QStringLiteral("performance_trend_diff"));

    const QJsonObject imageConfig = image.value(QStringLiteral("effective_config")).toObject();
    const QJsonObject imageTolerance = imageConfig.value(QStringLiteral("tolerances")).toObject();
    const QJsonObject performanceConfig = performance.value(QStringLiteral("effective_config")).toObject();
    const QJsonObject performanceThreshold = performanceConfig.value(QStringLiteral("thresholds")).toObject();
    const QJsonObject imageFailure = image.value(QStringLiteral("failure_report")).toObject();
    const QJsonArray imageBlockedBy = image.value(QStringLiteral("blocked_by")).toArray();

    if (!expect(policy.value(QStringLiteral("policy")).toString() == QStringLiteral("boundary_only"), "policy mode mismatch")
        || !expect(policy.value(QStringLiteral("opt_in_requested")).toBool(), "policy opt-in mismatch")
        || !expect(!policy.value(QStringLiteral("generation_performed")).toBool(), "generation should not run in N73")
        || !expect(topFailureReport.value(QStringLiteral("path")).toString() == QStringLiteral("artifacts/testdiff/generated/failure_report.json"), "top failure report path mismatch")
        || !expect(isCaseRelativePath(topFailureReport.value(QStringLiteral("path")).toString()), "top failure report path must be case-relative")
        || !expect(!image.isEmpty() && image.value(QStringLiteral("opt_in_requested")).toBool(), "image opt-in mismatch")
        || !expect(!image.value(QStringLiteral("enabled")).toBool(), "image generator must remain disabled")
        || !expect(image.value(QStringLiteral("candidate")).toBool(), "image candidate mismatch")
        || !expect(imageTolerance.value(QStringLiteral("pixel_abs")).toDouble() == 2.0, "image tolerance mismatch")
        || !expect(imageFailure.value(QStringLiteral("status")).toString() == QStringLiteral("not_written"), "image failure report status mismatch")
        || !expect(imageFailure.value(QStringLiteral("path")).toString() == QStringLiteral("artifacts/testdiff/generated/failure_report.json"), "image failure report path mismatch")
        || !expect(imageBlockedBy.contains(QStringLiteral("generator implementation unavailable")), "image blocked reason missing")
        || !expect(!performance.isEmpty() && performance.value(QStringLiteral("opt_in_requested")).toBool(), "performance opt-in mismatch")
        || !expect(!performance.value(QStringLiteral("enabled")).toBool(), "performance generator must remain disabled")
        || !expect(performanceThreshold.value(QStringLiteral("regression_percent")).toDouble() == 7.5, "performance threshold mismatch")
        || !expect(performanceThreshold.value(QStringLiteral("min_sample_count")).toInt() == 3, "performance sample threshold mismatch"))
    {
        return 1;
    }

    QTextStream(stdout) << "TESTDIFF_GENERATION_FAILURE_REPORT_SMOKE_OK\n";
    return 0;
}
