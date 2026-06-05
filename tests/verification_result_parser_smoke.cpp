#include "core/verify/VerificationResultParser.h"

#include <QCoreApplication>
#include <QTextStream>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    QTextStream err(stderr);

    const QVector<occtdebug::TestgridRow> rows =
        occtdebug::VerificationResultParser::parseTestgridText(QStringLiteral(
            "module run pass fail pass_rate\n"
            "Modeling 10 9 1 90%\n"
            "Total 10 9 1 90%\n"));
    const QVector<occtdebug::VerificationFailureDetail> testgridFailures =
        occtdebug::VerificationResultParser::failureDetailsForTestgridRows(rows, QStringLiteral("artifacts/testgrid_result.json"));
    if (testgridFailures.size() != 1 || testgridFailures.first().name != QStringLiteral("Modeling"))
    {
        err << "testgrid failure details did not skip aggregate rows\n";
        return 1;
    }

    const occtdebug::TestdiffSummary testdiff =
        occtdebug::VerificationResultParser::parseTestdiffText(QStringLiteral(
            "name status metric note\n"
            "geometry_images diff 2px edge-highlight-changed\n"
            "regression fail 1 needs-review\n"
            "performance pass +0.8% within-threshold\n"));
    const QVector<occtdebug::VerificationFailureDetail> testdiffFailures =
        occtdebug::VerificationResultParser::failureDetailsForTestdiff(testdiff, QStringLiteral("verification/testdiff_summary.txt"));
    if (testdiff.changedCount != 1 || testdiff.failedCount != 1 || testdiffFailures.size() != 2)
    {
        err << "testdiff failure details mismatch\n";
        return 2;
    }

    const QVector<occtdebug::TestgridRow> beforeRows =
        occtdebug::VerificationResultParser::parseTestgridText(QStringLiteral("Modeling 10 10 0 100%\n"));
    const QVector<occtdebug::TestgridRow> afterRows =
        occtdebug::VerificationResultParser::parseTestgridText(QStringLiteral("Modeling 10 9 1 90%\n"));
    const occtdebug::TestgridComparison comparison =
        occtdebug::VerificationResultParser::compareTestgridRows(beforeRows, afterRows);
    const QVector<occtdebug::VerificationFailureDetail> comparisonFailures =
        occtdebug::VerificationResultParser::failureDetailsForComparison(comparison, QStringLiteral("artifacts/testgrid_result.json"));
    if (!comparison.hasRegression() || comparisonFailures.size() != 1)
    {
        err << "comparison failure details mismatch\n";
        return 3;
    }

    const occtdebug::VerificationTimingSummary timing =
        occtdebug::VerificationResultParser::timingSummary({
            {QStringLiteral("draw_smoke_gate"), 40, QStringLiteral("passed")},
            {QStringLiteral("configured_testgrid"), 160, QStringLiteral("failed")},
        });
    if (timing.totalElapsedMs != 200 || timing.entries.size() != 2)
    {
        err << "timing summary mismatch\n";
        return 4;
    }

    out << "VERIFICATION_RESULT_PARSER_SMOKE_OK\n";
    return 0;
}
