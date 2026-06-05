#include "core/case/CaseManifest.h"
#include "core/verify/VerificationReportWriter.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTextStream>

#include <optional>

namespace
{
bool containsLocalAbsolutePath(const QString& text)
{
    const QString normalized = QString(text).replace(QLatin1Char('\\'), QLatin1Char('/'));
    return QRegularExpression(QStringLiteral(R"(\b[A-Za-z]:/)")).match(normalized).hasMatch();
}
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    QTextStream err(stderr);

    if (app.arguments().size() != 4)
    {
        err << "usage: verification_report_writer_smoke <case.json> <output.md> <output.json>\n";
        return 2;
    }

    const QString manifestPath = app.arguments().at(1);
    const QString markdownPath = app.arguments().at(2);
    const QString jsonPath = app.arguments().at(3);
    const QString caseRoot = QFileInfo(manifestPath).absolutePath();

    QString error;
    const std::optional<occtdebug::CaseManifest> manifest = occtdebug::CaseManifest::loadFromFile(manifestPath, &error);
    if (!manifest.has_value())
    {
        err << "failed to load manifest: " << error << "\n";
        return 3;
    }

    if (!occtdebug::VerificationReportWriter::writeReport(*manifest, caseRoot, markdownPath, jsonPath, &error))
    {
        err << "failed to write verification report: " << error << "\n";
        return 4;
    }

    QFile jsonFile(jsonPath);
    if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        err << "failed to reopen verification json: " << jsonFile.errorString() << "\n";
        return 5;
    }

    const QByteArray jsonBytes = jsonFile.readAll();
    const QJsonObject root = QJsonDocument::fromJson(jsonBytes).object();
    if (root.value(QStringLiteral("schema_version")).toInt() != 1)
    {
        err << "schema_version mismatch\n";
        return 6;
    }
    if (root.value(QStringLiteral("case_id")).toString() != manifest->caseId)
    {
        err << "case_id mismatch\n";
        return 7;
    }
    if (root.value(QStringLiteral("gate")).toObject().isEmpty())
    {
        err << "gate object is empty\n";
        return 8;
    }
    if (!root.value(QStringLiteral("gate")).toObject().contains(QStringLiteral("patch_dry_run")))
    {
        err << "patch_dry_run gate is missing\n";
        return 9;
    }
    if (!root.value(QStringLiteral("gate")).toObject().contains(QStringLiteral("patch_review")))
    {
        err << "patch_review gate is missing\n";
        return 10;
    }
    if (!root.value(QStringLiteral("gate")).toObject().contains(QStringLiteral("patch_signoff")))
    {
        err << "patch_signoff gate is missing\n";
        return 11;
    }
    const QJsonObject patch = root.value(QStringLiteral("patch")).toObject();
    const QJsonObject patchDecision = patch.value(QStringLiteral("decision")).toObject();
    if (patchDecision.isEmpty())
    {
        err << "patch decision is missing\n";
        return 12;
    }
    if (patchDecision.value(QStringLiteral("state")).toString() != QStringLiteral("approved")
        || patchDecision.value(QStringLiteral("gate_state")).toString() != QStringLiteral("blocked"))
    {
        err << "patch decision did not bind approved review to failed verification\n";
        return 13;
    }
    if (patch.value(QStringLiteral("signoff_status")).toString() != QStringLiteral("blocked"))
    {
        err << "patch signoff status mismatch\n";
        return 14;
    }
    if (root.value(QStringLiteral("artifacts")).toObject().value(QStringLiteral("verification_report_json")).toString()
        != QStringLiteral("verification/verification_report.json"))
    {
        err << "verification json artifact path mismatch\n";
        return 15;
    }
    if (root.value(QStringLiteral("artifacts")).toObject().value(QStringLiteral("patch_generate_result")).toString()
        != QStringLiteral("artifacts/patch_generate_result.json"))
    {
        err << "patch generation artifact path mismatch\n";
        return 31;
    }
    if (root.value(QStringLiteral("failure_details")).toArray().isEmpty())
    {
        err << "failure_details are empty\n";
        return 16;
    }
    if (root.value(QStringLiteral("timing")).toObject().value(QStringLiteral("entries")).toArray().isEmpty())
    {
        err << "timing entries are empty\n";
        return 26;
    }
    const QJsonObject testdiff = root.value(QStringLiteral("testdiff")).toObject();
    if (testdiff.value(QStringLiteral("artifacts")).toObject().value(QStringLiteral("summary")).toString()
        != QStringLiteral("verification/testdiff_summary.txt"))
    {
        err << "testdiff artifact summary mismatch\n";
        return 27;
    }
    const QJsonObject testdiffArtifacts = testdiff.value(QStringLiteral("artifacts")).toObject();
    if (testdiffArtifacts.value(QStringLiteral("artifact_files")).toArray().size() < 3
        || testdiffArtifacts.value(QStringLiteral("artifact_counts")).toObject().value(QStringLiteral("image")).toInt() != 1)
    {
        err << "testdiff detailed artifact manifest mismatch\n";
        return 33;
    }
    const QJsonObject artifactIndex = testdiffArtifacts.value(QStringLiteral("artifact_index")).toObject();
    const QJsonObject artifactIndexSummary = testdiffArtifacts.value(QStringLiteral("artifact_index_summary")).toObject();
    if (artifactIndex.value(QStringLiteral("groups")).toArray().size() != 3
        || artifactIndexSummary.value(QStringLiteral("total_groups")).toInt() != 3
        || artifactIndexSummary.value(QStringLiteral("status_counts")).toObject().value(QStringLiteral("diff_only")).toInt() != 3
        || artifactIndexSummary.value(QStringLiteral("kind_group_counts")).toObject().value(QStringLiteral("image")).toInt() != 1)
    {
        err << "testdiff artifact index summary mismatch\n";
        return 37;
    }
    const QJsonObject artifactAnalysis = testdiffArtifacts.value(QStringLiteral("artifact_analysis")).toObject();
    const QJsonObject artifactAnalysisSummary = artifactAnalysis.value(QStringLiteral("summary")).toObject();
    const QJsonObject generationPolicy = artifactAnalysis.value(QStringLiteral("generation_policy")).toObject();
    if (artifactAnalysisSummary.value(QStringLiteral("property_json_parsed")).toInt() != 1
        || artifactAnalysisSummary.value(QStringLiteral("performance_metrics")).toInt() != 1
        || generationPolicy.value(QStringLiteral("policy")).toString() != QStringLiteral("boundary_only")
        || generationPolicy.value(QStringLiteral("generation_performed")).toBool())
    {
        err << "testdiff artifact analysis summary mismatch\n";
        return 39;
    }
    bool foundTestdiffFailure = false;
    for (const QJsonValue& value : root.value(QStringLiteral("failure_details")).toArray())
    {
        const QJsonObject failure = value.toObject();
        foundTestdiffFailure = foundTestdiffFailure
            || failure.value(QStringLiteral("type")).toString() == QStringLiteral("testdiff");
    }
    if (!foundTestdiffFailure)
    {
        err << "testdiff failure detail is missing\n";
        return 28;
    }
    const QJsonObject beforeAfter = root.value(QStringLiteral("before_after")).toObject();
    if (!beforeAfter.value(QStringLiteral("available")).toBool())
    {
        err << "before_after comparison is unavailable\n";
        return 17;
    }
    if (beforeAfter.value(QStringLiteral("fail_delta")).toInt() >= 0)
    {
        err << "before_after fail delta did not improve\n";
        return 18;
    }
    const QJsonObject geometryDiff = root.value(QStringLiteral("geometry_diff")).toObject();
    if (!geometryDiff.value(QStringLiteral("available")).toBool()
        || geometryDiff.value(QStringLiteral("artifact")).toString() != QStringLiteral("artifacts/topology_compare.json")
        || geometryDiff.value(QStringLiteral("match_summary")).toObject().value(QStringLiteral("matched")).toInt() <= 0)
    {
        err << "geometry diff topology artifact mismatch\n";
        return 35;
    }
    if (containsLocalAbsolutePath(QString::fromUtf8(jsonBytes)))
    {
        err << "verification json contains a local absolute path\n";
        return 19;
    }

    QFile markdownFile(markdownPath);
    if (!markdownFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        err << "failed to reopen verification markdown: " << markdownFile.errorString() << "\n";
        return 20;
    }
    const QString markdown = QString::fromUtf8(markdownFile.readAll());
    if (!markdown.contains(QStringLiteral("# Verification Report")))
    {
        err << "markdown title missing\n";
        return 21;
    }
    if (!markdown.contains(QStringLiteral("## Before / After Comparison")))
    {
        err << "before/after markdown section missing\n";
        return 22;
    }
    if (!markdown.contains(QStringLiteral("patch_review")))
    {
        err << "patch_review markdown gate missing\n";
        return 23;
    }
    if (!markdown.contains(QStringLiteral("patch_signoff")))
    {
        err << "patch_signoff markdown gate missing\n";
        return 24;
    }
    if (!markdown.contains(QStringLiteral("## Timing Summary")))
    {
        err << "timing markdown section missing\n";
        return 29;
    }
    if (!markdown.contains(QStringLiteral("Testdiff summary")))
    {
        err << "testdiff artifact markdown link missing\n";
        return 30;
    }
    if (!markdown.contains(QStringLiteral("Patch generation result")))
    {
        err << "patch generation artifact markdown link missing\n";
        return 32;
    }
    if (!markdown.contains(QStringLiteral("Testdiff image"))
        || !markdown.contains(QStringLiteral("artifacts/testdiff/diff/geometry_images.png")))
    {
        err << "testdiff detailed artifact markdown link missing\n";
        return 34;
    }
    if (!markdown.contains(QStringLiteral("## Testdiff Artifact Index"))
        || !markdown.contains(QStringLiteral("diff_only"))
        || !markdown.contains(QStringLiteral("performance_timing")))
    {
        err << "testdiff artifact index markdown section missing\n";
        return 38;
    }
    if (!markdown.contains(QStringLiteral("## Testdiff Artifact Analysis"))
        || !markdown.contains(QStringLiteral("Property JSON parsed"))
        || !markdown.contains(QStringLiteral("Performance metrics extracted")))
    {
        err << "testdiff artifact analysis markdown section missing\n";
        return 40;
    }
    if (!markdown.contains(QStringLiteral("## Geometry Diff"))
        || !markdown.contains(QStringLiteral("Topology compare"))
        || !markdown.contains(QStringLiteral("artifacts/topology_compare.json")))
    {
        err << "geometry diff markdown section missing\n";
        return 36;
    }
    if (containsLocalAbsolutePath(markdown))
    {
        err << "verification markdown contains a local absolute path\n";
        return 25;
    }

    out << "VERIFICATION_REPORT_SMOKE_OK " << QDir::toNativeSeparators(jsonPath) << "\n";
    return 0;
}
