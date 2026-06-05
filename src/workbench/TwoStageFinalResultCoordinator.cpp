#include "workbench/TwoStageFinalResultCoordinator.h"

#include "workbench/EvidenceCoordinator.h"

namespace occtdebug
{
namespace
{
struct TestgridTotals
{
    int run = 0;
    int pass = 0;
    int fail = 0;
};

TestgridTotals totalsForRows(const QVector<TestgridRow>& rows)
{
    TestgridTotals totals;
    for (const TestgridRow& row : rows)
    {
        totals.run += row.runCount.toInt();
        totals.pass += row.passCount.toInt();
        totals.fail += row.failCount.toInt();
    }
    return totals;
}

QString testdiffSummaryText(const TestdiffSummary& testdiff)
{
    return testdiff.entries.isEmpty() ? QStringLiteral("testdiff not available") : testdiff.summaryText();
}
} // namespace

TwoStageFinalResultSyncResult TwoStageFinalResultCoordinator::sync(
    WorkbenchMockData& data,
    const TwoStageFinalResultSyncInput& input)
{
    const TestgridTotals totals = totalsForRows(input.afterRows);
    TwoStageFinalResultSyncResult result;
    result.diffSummary = QStringLiteral("%1\n%2")
        .arg(input.comparison.summaryText(), testdiffSummaryText(input.testdiff));
    result.verificationItems = {
        {QStringLiteral("two-stage verification"), QStringLiteral("%1: %2").arg(input.finalStatus, input.note)},
        {QStringLiteral("testgrid runner"), (input.beforeCommandExecuted || input.afterCommandExecuted) ? QStringLiteral("executed two-stage") : QStringLiteral("skipped two-stage")},
        {QStringLiteral("patch apply"), data.patchApplyStatus},
        {QStringLiteral("testgrid"), QStringLiteral("%1 / %2 passed, %3 failed").arg(totals.pass).arg(totals.run).arg(totals.fail)},
        {QStringLiteral("before/after"), input.comparison.summaryText()},
        {QStringLiteral("testdiff"), testdiffSummaryText(input.testdiff)},
    };
    result.evidence = {
        QStringLiteral("Verification"),
        QStringLiteral("Two-stage testgrid verification"),
        QStringLiteral("status=%1 failures=%2 elapsed=%3ms %4")
            .arg(input.finalStatus)
            .arg(input.failureDetails.size())
            .arg(input.timing.totalElapsedMs)
            .arg(input.comparison.summaryText()),
        QStringLiteral("artifacts/testgrid_two_stage_result.json"),
    };

    data.testgridRows = input.afterRows;
    data.manifest.testgridRows = input.afterRows;
    data.diffSummary = result.diffSummary;
    data.manifest.diffSummary = result.diffSummary;
    data.verificationItems = result.verificationItems;
    data.manifest.verificationItems = result.verificationItems;
    EvidenceCoordinator::appendRecord(data, result.evidence);

    return result;
}
} // namespace occtdebug
