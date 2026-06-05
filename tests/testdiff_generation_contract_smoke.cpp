#include "core/verify/TestdiffGenerationContract.h"

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

bool isCaseRelativePattern(const QString& value)
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

    const QJsonObject contract = occtdebug::TestdiffGenerationContract::build();
    const QString outputRoot = contract.value(QStringLiteral("output_root")).toString();
    const QString sidecarSuffix = contract.value(QStringLiteral("sidecar_suffix")).toString();
    const QJsonObject defaultConfig = contract.value(QStringLiteral("default_config")).toObject();
    const QJsonObject failureReport = contract.value(QStringLiteral("failure_report")).toObject();
    const QJsonArray generators = contract.value(QStringLiteral("generators")).toArray();

    if (!expect(contract.value(QStringLiteral("mode")).toString() == QStringLiteral("opt_in"), "contract mode mismatch")
        || !expect(!contract.value(QStringLiteral("enabled_by_default")).toBool(true), "contract should be disabled by default")
        || !expect(contract.value(QStringLiteral("config_manifest_field")).toString() == QStringLiteral("verification.testdiff_generation"), "config manifest field mismatch")
        || !expect(contract.value(QStringLiteral("case_manifest_field")).toString() == QStringLiteral("verification.testdiff_generation.enabled_generators"), "manifest field mismatch")
        || !expect(outputRoot == QStringLiteral("artifacts/testdiff/generated"), "output root mismatch")
        || !expect(isCaseRelativePattern(outputRoot), "output root must be case-relative")
        || !expect(sidecarSuffix == QStringLiteral(".meta.json"), "sidecar suffix mismatch")
        || !expect(defaultConfig.value(QStringLiteral("enabled_generators")).toArray().isEmpty(), "default generators should be empty")
        || !expect(defaultConfig.value(QStringLiteral("failure_report")).toObject().value(QStringLiteral("path")).toString() == QStringLiteral("artifacts/testdiff/generated/failure_report.json"), "default failure report path mismatch")
        || !expect(failureReport.value(QStringLiteral("path")).toString() == QStringLiteral("artifacts/testdiff/generated/failure_report.json"), "failure report path mismatch")
        || !expect(isCaseRelativePattern(failureReport.value(QStringLiteral("path")).toString()), "failure report path must be case-relative")
        || !expect(failureReport.value(QStringLiteral("issue_fields")).toArray().contains(QStringLiteral("blocked_by")), "failure report fields missing blocked_by")
        || !expect(generators.size() == 3, "generator contract count mismatch")
        || !expect(contract.value(QStringLiteral("privacy_rules")).toArray().size() >= 3, "privacy rules missing"))
    {
        return 1;
    }

    for (const QJsonValue& value : generators)
    {
        const QJsonObject generator = value.toObject();
        const QString id = generator.value(QStringLiteral("id")).toString();
        const QString outputPattern = generator.value(QStringLiteral("output_pattern")).toString();
        const QString sidecarPattern = generator.value(QStringLiteral("sidecar_pattern")).toString();
        if (!expect(!id.isEmpty(), "generator id missing")
            || !expect(generator.value(QStringLiteral("generated_role")).toString() == QStringLiteral("diff"), "generated role mismatch")
            || !expect(generator.value(QStringLiteral("required_inputs")).toArray().size() >= 2, "required inputs missing")
            || !expect(outputPattern.startsWith(outputRoot + QLatin1Char('/')), "output pattern outside output root")
            || !expect(sidecarPattern.startsWith(outputRoot + QLatin1Char('/')), "sidecar pattern outside output root")
            || !expect(isCaseRelativePattern(outputPattern), "output pattern must be case-relative")
            || !expect(isCaseRelativePattern(sidecarPattern), "sidecar pattern must be case-relative")
            || !expect(sidecarPattern.endsWith(sidecarSuffix), "sidecar pattern suffix mismatch")
            || !expect(generator.value(QStringLiteral("sidecar_fields")).toArray().contains(QStringLiteral("input_artifacts")), "sidecar input fields missing")
            || !expect(generator.value(QStringLiteral("sidecar_fields")).toArray().contains(QStringLiteral("status")), "sidecar status field missing"))
        {
            return 2;
        }
    }

    QTextStream(stdout) << "TESTDIFF_GENERATION_CONTRACT_SMOKE_OK\n";
    return 0;
}
