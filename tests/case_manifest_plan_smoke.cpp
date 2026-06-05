#include "core/case/CaseManifest.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QTextStream>

#include <optional>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    if (app.arguments().size() != 2)
    {
        QTextStream(stderr) << "usage: case_manifest_plan_smoke <case.json>\n";
        return 2;
    }

    QString error;
    const std::optional<occtdebug::CaseManifest> manifest =
        occtdebug::CaseManifest::loadFromFile(app.arguments().at(1), &error);
    if (!manifest.has_value())
    {
        QTextStream(stderr) << "failed to load manifest: " << error << "\n";
        return 3;
    }
    if (manifest->verificationPlan.testdiffArguments != QStringLiteral("--group {group} --grid {grid} --case {case} --out {output}"))
    {
        QTextStream(stderr) << "testdiff arguments mismatch\n";
        return 4;
    }
    if (!manifest->verificationPlan.testdiffOutputRoot.isEmpty())
    {
        QTextStream(stderr) << "testdiff output root should be empty in sample case\n";
        return 5;
    }

    const QJsonObject json = manifest->toJson();
    const QJsonObject verification = json.value(QStringLiteral("verification")).toObject();
    const QJsonObject plan = verification.value(QStringLiteral("testgrid_plan")).toObject();
    if (plan.value(QStringLiteral("testdiff_arguments")).toString() != manifest->verificationPlan.testdiffArguments
        || plan.value(QStringLiteral("testdiff_output_root")).toString() != manifest->verificationPlan.testdiffOutputRoot)
    {
        QTextStream(stderr) << "testdiff plan fields were not serialized\n";
        return 6;
    }

    occtdebug::CaseManifest withInput = *manifest;
    withInput.inputFiles = {
        {
            QStringLiteral("input/demo.brep"),
            QStringLiteral("demo.brep"),
            QStringLiteral("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"),
            128,
            QStringLiteral("2026-06-05T00:00:00Z"),
        },
    };
    const QJsonObject inputObject = withInput.toJson().value(QStringLiteral("input")).toObject();
    const QJsonArray files = inputObject.value(QStringLiteral("files")).toArray();
    if (files.size() != 1)
    {
        QTextStream(stderr) << "input files were not serialized\n";
        return 7;
    }
    const QJsonObject inputFile = files.first().toObject();
    if (inputFile.value(QStringLiteral("path")).toString() != QStringLiteral("input/demo.brep")
        || inputFile.value(QStringLiteral("sha256")).toString() != withInput.inputFiles.first().sha256
        || inputFile.value(QStringLiteral("bytes")).toInt() != 128)
    {
        QTextStream(stderr) << "input file fields mismatch\n";
        return 8;
    }
    QString roundTripError;
    const std::optional<occtdebug::CaseManifest> roundTrip =
        occtdebug::CaseManifest::fromJson(withInput.toJson(), &roundTripError);
    if (!roundTrip.has_value() || roundTrip->inputFiles.size() != 1
        || roundTrip->inputFiles.first().path != QStringLiteral("input/demo.brep")
        || roundTrip->inputFiles.first().sha256 != withInput.inputFiles.first().sha256)
    {
        QTextStream(stderr) << "input files did not round-trip: " << roundTripError << "\n";
        return 9;
    }

    withInput.reproStatus.overall = QStringLiteral("reproduced");
    withInput.reproStatus.draw = QStringLiteral("passed");
    withInput.reproStatus.cpp = QStringLiteral("generated");
    withInput.reproStatus.testgrid = QStringLiteral("unknown");
    withInput.reproStatus.updatedAt = QStringLiteral("2026-06-05T00:00:01Z");
    withInput.reproStatus.summary = QStringLiteral("draw=passed cpp=generated");
    const QJsonObject reproObject = withInput.toJson().value(QStringLiteral("repro")).toObject();
    const QJsonObject reproStatus = reproObject.value(QStringLiteral("status")).toObject();
    if (reproStatus.value(QStringLiteral("overall")).toString() != QStringLiteral("reproduced")
        || reproStatus.value(QStringLiteral("draw")).toString() != QStringLiteral("passed"))
    {
        QTextStream(stderr) << "repro status was not serialized\n";
        return 10;
    }
    QString reproRoundTripError;
    const std::optional<occtdebug::CaseManifest> reproRoundTrip =
        occtdebug::CaseManifest::fromJson(withInput.toJson(), &reproRoundTripError);
    if (!reproRoundTrip.has_value()
        || reproRoundTrip->reproStatus.overall != QStringLiteral("reproduced")
        || reproRoundTrip->reproStatus.cpp != QStringLiteral("generated"))
    {
        QTextStream(stderr) << "repro status did not round-trip: " << reproRoundTripError << "\n";
        return 11;
    }

    QTextStream(stdout) << "CASE_MANIFEST_PLAN_SMOKE_OK\n";
    return 0;
}
