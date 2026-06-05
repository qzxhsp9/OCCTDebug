#include "core/verify/TestdiffGenerationPolicy.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
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
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    const QJsonObject artifactIndex {
        {QStringLiteral("groups"), QJsonArray {
             QJsonObject {
                 {QStringLiteral("kind"), QStringLiteral("image")},
                 {QStringLiteral("key"), QStringLiteral("view")},
                 {QStringLiteral("status"), QStringLiteral("paired")},
                 {QStringLiteral("before"), QStringLiteral("artifacts/testdiff/before/view.png")},
                 {QStringLiteral("after"), QStringLiteral("artifacts/testdiff/after/view.png")},
             },
             QJsonObject {
                 {QStringLiteral("kind"), QStringLiteral("property")},
                 {QStringLiteral("key"), QStringLiteral("props")},
                 {QStringLiteral("status"), QStringLiteral("paired_with_diff")},
                 {QStringLiteral("before"), QStringLiteral("artifacts/testdiff/before/props.json")},
                 {QStringLiteral("after"), QStringLiteral("artifacts/testdiff/after/props.json")},
                 {QStringLiteral("diff"), QStringLiteral("artifacts/testdiff/diff/props.json")},
             },
             QJsonObject {
                 {QStringLiteral("kind"), QStringLiteral("performance")},
                 {QStringLiteral("key"), QStringLiteral("timing")},
                 {QStringLiteral("status"), QStringLiteral("diff_only")},
                 {QStringLiteral("diff"), QStringLiteral("artifacts/testdiff/diff/timing.txt")},
             },
         }},
    };
    const QJsonObject artifactAnalysis {
        {QStringLiteral("summary"), QJsonObject {
             {QStringLiteral("property_json_parsed"), 1},
             {QStringLiteral("performance_metrics"), 3},
         }},
    };

    const QJsonObject policy = occtdebug::TestdiffGenerationPolicy::build(artifactIndex, artifactAnalysis);
    const QJsonObject contract = policy.value(QStringLiteral("contract")).toObject();
    const QJsonArray generators = policy.value(QStringLiteral("generators")).toArray();
    const QJsonObject image = findGenerator(generators, QStringLiteral("image_pixel_diff"));
    const QJsonObject property = findGenerator(generators, QStringLiteral("property_structural_diff"));
    const QJsonObject performance = findGenerator(generators, QStringLiteral("performance_trend_diff"));

    if (!expect(policy.value(QStringLiteral("policy")).toString() == QStringLiteral("boundary_only"), "policy mode mismatch")
        || !expect(!policy.value(QStringLiteral("generation_performed")).toBool(), "policy generation flag mismatch")
        || !expect(contract.value(QStringLiteral("mode")).toString() == QStringLiteral("opt_in"), "contract mode mismatch")
        || !expect(!contract.value(QStringLiteral("enabled_by_default")).toBool(true), "contract default mismatch")
        || !expect(contract.value(QStringLiteral("generators")).toArray().size() == 3, "contract generator count mismatch")
        || !expect(generators.size() == 3, "generator count mismatch")
        || !expect(!image.isEmpty() && image.value(QStringLiteral("candidate")).toBool(), "image candidate mismatch")
        || !expect(!image.value(QStringLiteral("enabled")).toBool(), "image generator should be disabled")
        || !expect(image.value(QStringLiteral("current_inputs")).toObject().value(QStringLiteral("paired_groups")).toInt() == 1, "image paired input mismatch")
        || !expect(!property.isEmpty() && property.value(QStringLiteral("candidate")).toBool(), "property candidate mismatch")
        || !expect(!property.value(QStringLiteral("enabled")).toBool(), "property generator should be disabled")
        || !expect(property.value(QStringLiteral("current_inputs")).toObject().value(QStringLiteral("parsed_property_groups")).toInt() == 1, "property parsed count mismatch")
        || !expect(!performance.isEmpty() && performance.value(QStringLiteral("candidate")).toBool(), "performance candidate mismatch")
        || !expect(!performance.value(QStringLiteral("enabled")).toBool(), "performance generator should be disabled")
        || !expect(performance.value(QStringLiteral("current_inputs")).toObject().value(QStringLiteral("parsed_metric_count")).toInt() == 3, "performance metric count mismatch")
        || !expect(policy.value(QStringLiteral("next_steps")).toArray().size() >= 3, "next steps missing"))
    {
        return 1;
    }

    QTextStream(stdout) << "TESTDIFF_GENERATION_POLICY_SMOKE_OK\n";
    return 0;
}
