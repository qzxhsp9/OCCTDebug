#include "workbench/CaseManifestSync.h"
#include "workbench/DiffArtifactsPresenter.h"
#include "workbench/DiffPanel.h"
#include "workbench/EvidenceCoordinator.h"
#include "workbench/EvidencePanel.h"
#include "workbench/TestdiffAdapterResultCoordinator.h"
#include "workbench/TestgridTablePresenter.h"
#include "workbench/TwoStageFinalResultCoordinator.h"
#include "workbench/TwoStageFinalResultUiAdapter.h"
#include "workbench/VerificationPanel.h"

#include <QApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QPixmap>
#include <QTableWidget>
#include <QTextStream>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    occtdebug::WorkbenchMockData data;
    occtdebug::EvidenceRecord evidence;
    evidence.type = QStringLiteral("Geometry");
    evidence.title = QStringLiteral("Picked edge");
    evidence.summary = QStringLiteral("edge selected in viewer");
    evidence.link = QStringLiteral("artifacts/geometry_selection.json");
    evidence.geometryObject = QStringLiteral("E1");

    occtdebug::EvidenceCoordinator::appendRecord(data, evidence);
    if (data.evidenceItems.size() != 1
        || data.manifest.evidenceItems.size() != 1
        || data.evidenceItems.first().geometryObject != QStringLiteral("E1"))
    {
        QTextStream(stderr) << "evidence coordinator did not sync data and manifest\n";
        return 1;
    }

    occtdebug::WorkbenchMockData syncData;
    syncData.reproScript = QStringLiteral("pload MODELING");
    syncData.reproStatus.overall = QStringLiteral("reproduced");
    syncData.environmentSummary = QStringLiteral("env captured");
    syncData.geometrySummary = QStringLiteral("shape loaded");
    syncData.geometryChecks = {{QStringLiteral("checkshape"), QStringLiteral("valid"), QStringLiteral("ok")}};
    syncData.evidenceItems = {evidence};
    syncData.verificationItems = {{QStringLiteral("draw"), QStringLiteral("passed")}};
    syncData.verificationPlan.testdiffArguments = QStringLiteral("--case {case}");
    syncData.testdiffGenerationConfig.enabledGenerators = {QStringLiteral("image_pixel_diff")};
    syncData.testdiffGenerationConfig.imagePixelTolerance = 1.0;
    syncData.patchReviewStatus = QStringLiteral("approved");
    syncData.patchWorktreeRoot = QStringLiteral("occt-worktree");
    syncData.patchApplyStatus = QStringLiteral("apply passed");
    syncData.patchApplyLog = QStringLiteral("patch log");
    syncData.patchSignoffStatus = QStringLiteral("signed off");
    syncData.patchSignoffNote = QStringLiteral("ready");
    syncData.patchReviewItems = {{QStringLiteral("risk"), QStringLiteral("low")}};
    syncData.testgridRows = {{QStringLiteral("bugs modalg_1"), QStringLiteral("1"), QStringLiteral("1"), QStringLiteral("0"), QStringLiteral("100%")}};
    syncData.workflowSteps = {{QStringLiteral("input"), QStringLiteral("1"), QStringLiteral("Input"), QStringLiteral("done"), QStringLiteral("ok")}};

    occtdebug::CaseManifest syncedManifest;
    occtdebug::CaseManifestSync::syncMutableFields(syncedManifest, syncData);
    if (syncedManifest.reproScript != syncData.reproScript
        || syncedManifest.reproStatus.overall != QStringLiteral("reproduced")
        || syncedManifest.geometryChecks.size() != 1
        || syncedManifest.evidenceItems.size() != 1
        || syncedManifest.verificationPlan.testdiffArguments != QStringLiteral("--case {case}")
        || syncedManifest.testdiffGenerationConfig.enabledGenerators.size() != 1
        || syncedManifest.patchReviewStatus != QStringLiteral("approved")
        || syncedManifest.patchSignoffStatus != QStringLiteral("signed off")
        || syncedManifest.testgridRows.size() != 1
        || syncedManifest.workflowState.activeStepId != QStringLiteral("input")
        || syncedManifest.workflowSteps.size() != 1)
    {
        QTextStream(stderr) << "case manifest sync did not copy mutable workbench fields\n";
        return 1;
    }

    QVector<occtdebug::TestgridRow> rows {
        {QStringLiteral("bugs modalg_1"), QStringLiteral("3"), QStringLiteral("2"), QStringLiteral("1"), QStringLiteral("66.7%")},
        {QStringLiteral("draw smoke"), QStringLiteral("1"), QStringLiteral("1"), QStringLiteral("0"), QStringLiteral("100%")},
    };
    const QVector<QStringList> cells = occtdebug::TestgridTablePresenter::rowsToCells(rows);
    if (cells.size() != 2
        || cells[0].size() != 5
        || cells[0][0] != QStringLiteral("bugs modalg_1")
        || cells[0][3] != QStringLiteral("1")
        || cells[1][4] != QStringLiteral("100%"))
    {
        QTextStream(stderr) << "testgrid presenter produced unexpected cells\n";
        return 1;
    }

    const QJsonObject testdiffArtifacts {
        {QStringLiteral("artifact_index"),
         QJsonObject {
             {QStringLiteral("groups"),
              QJsonArray {
                  QJsonObject {
                      {QStringLiteral("kind"), QStringLiteral("image")},
                      {QStringLiteral("key"), QStringLiteral("view")},
                      {QStringLiteral("status"), QStringLiteral("paired_with_diff")},
                      {QStringLiteral("before"), QStringLiteral("artifacts/testdiff/before/view.png")},
                      {QStringLiteral("after"), QStringLiteral("artifacts/testdiff/after/view.png")},
                      {QStringLiteral("diff"), QStringLiteral("artifacts/testdiff/diff/view.png")},
                  },
                  QJsonObject {
                      {QStringLiteral("kind"), QStringLiteral("performance")},
                      {QStringLiteral("key"), QStringLiteral("timing")},
                      {QStringLiteral("status"), QStringLiteral("diff_only")},
                      {QStringLiteral("diff"), QStringLiteral("artifacts/testdiff/diff/timing.txt")},
                  },
              }},
         }},
        {QStringLiteral("artifact_analysis"),
         QJsonObject {
             {QStringLiteral("groups"),
              QJsonArray {
                  QJsonObject {
                      {QStringLiteral("kind"), QStringLiteral("property")},
                      {QStringLiteral("key"), QStringLiteral("props")},
                      {QStringLiteral("status"), QStringLiteral("diff_only")},
                      {QStringLiteral("analysis"),
                       QJsonObject {
                           {QStringLiteral("json"),
                            QJsonObject {
                                {QStringLiteral("json_type"), QStringLiteral("object")},
                                {QStringLiteral("top_level_key_count"), 2},
                                {QStringLiteral("top_level_keys"),
                                 QJsonArray {QStringLiteral("faces"), QStringLiteral("edges")}},
                            }},
                       }},
                  },
                  QJsonObject {
                      {QStringLiteral("kind"), QStringLiteral("performance")},
                      {QStringLiteral("key"), QStringLiteral("timing")},
                      {QStringLiteral("status"), QStringLiteral("diff_only")},
                      {QStringLiteral("analysis"),
                       QJsonObject {
                           {QStringLiteral("metrics"),
                            QJsonArray {
                                QJsonObject {
                                    {QStringLiteral("name"), QStringLiteral("fillet")},
                                    {QStringLiteral("value"), 0.8},
                                    {QStringLiteral("unit"), QStringLiteral("%")},
                                },
                            }},
                       }},
                  },
              }},
         }},
    };
    const QVector<QStringList> indexRows = occtdebug::DiffArtifactsPresenter::indexRows(testdiffArtifacts);
    const QVector<QStringList> analysisRows = occtdebug::DiffArtifactsPresenter::analysisRows(testdiffArtifacts);
    const QVector<QStringList> imageRows = occtdebug::DiffArtifactsPresenter::indexRows(
        testdiffArtifacts,
        occtdebug::DiffArtifactsPresenter::Filter {QStringLiteral("image"), QString()});
    const QVector<QStringList> diffOnlyRows = occtdebug::DiffArtifactsPresenter::analysisRows(
        testdiffArtifacts,
        occtdebug::DiffArtifactsPresenter::Filter {QString(), QStringLiteral("diff_only")});
    const QVector<QStringList> searchRows = occtdebug::DiffArtifactsPresenter::indexRows(
        testdiffArtifacts,
        occtdebug::DiffArtifactsPresenter::Filter {QString(), QString(), QStringLiteral("timing.txt")});
    if (indexRows.size() != 2
        || indexRows[0][0] != QStringLiteral("image")
        || indexRows[0][5] != QStringLiteral("artifacts/testdiff/diff/view.png")
        || analysisRows.size() != 2
        || !analysisRows[0][3].contains(QStringLiteral("faces"))
        || !analysisRows[1][3].contains(QStringLiteral("metrics=1"))
        || imageRows.size() != 1
        || imageRows[0][1] != QStringLiteral("view")
        || diffOnlyRows.size() != 2
        || searchRows.size() != 1
        || searchRows[0][1] != QStringLiteral("timing"))
    {
        QTextStream(stderr) << "diff artifacts presenter produced unexpected cells\n";
        return 1;
    }

    occtdebug::DiffPanel diffPanel;
    diffPanel.setDiffSummary(QStringLiteral("diff summary smoke"));
    diffPanel.setTestdiffArtifacts(testdiffArtifacts);
    QPixmap previewPixmap(24, 12);
    previewPixmap.fill(Qt::red);
    diffPanel.setPreviewImage(QStringLiteral("artifacts/testdiff/diff/view.png"), previewPixmap);
    if (diffPanel.summaryLabel()->text() != QStringLiteral("diff summary smoke")
        || diffPanel.indexRowCount() != 2
        || diffPanel.analysisRowCount() != 2
        || diffPanel.previewText() != QStringLiteral("artifacts/testdiff/diff/view.png")
        || diffPanel.preferredArtifactPath(QStringLiteral("image"), QStringLiteral("view"), QStringLiteral("paired_with_diff"))
            != QStringLiteral("artifacts/testdiff/diff/view.png")
        || diffPanel.preferredArtifactPath(QStringLiteral("performance"), QStringLiteral("timing"))
            != QStringLiteral("artifacts/testdiff/diff/timing.txt"))
    {
        QTextStream(stderr) << "diff panel did not refresh summary, rows, and preferred artifact paths\n";
        return 1;
    }

    occtdebug::WorkbenchMockData twoStageData;
    twoStageData.caseId = QStringLiteral("OCC-LOCAL-SMOKE");
    twoStageData.patchApplyStatus = QStringLiteral("applied");

    const QVector<occtdebug::TestgridRow> beforeRows {
        {QStringLiteral("bugs modalg_1"), QStringLiteral("3"), QStringLiteral("2"), QStringLiteral("1"), QStringLiteral("66.7%")},
        {QStringLiteral("bugs modalg_2"), QStringLiteral("1"), QStringLiteral("1"), QStringLiteral("0"), QStringLiteral("100%")},
    };
    const QVector<occtdebug::TestgridRow> afterRows {
        {QStringLiteral("bugs modalg_1"), QStringLiteral("3"), QStringLiteral("2"), QStringLiteral("1"), QStringLiteral("66.7%")},
        {QStringLiteral("bugs modalg_2"), QStringLiteral("1"), QStringLiteral("0"), QStringLiteral("1"), QStringLiteral("0%")},
    };
    const occtdebug::TestgridComparison comparison =
        occtdebug::VerificationResultParser::compareTestgridRows(beforeRows, afterRows);

    occtdebug::TestdiffSummary testdiff;
    testdiff.entries = {
        {QStringLiteral("image view"), QStringLiteral("changed"), QStringLiteral("pixels changed")},
    };
    testdiff.changedCount = 1;

    const occtdebug::VerificationTimingSummary timing {
        {
            {QStringLiteral("before_draw_smoke_gate"), 10, QStringLiteral("passed")},
            {QStringLiteral("after_configured_testgrid"), 32, QStringLiteral("failed")},
        },
        42,
    };

    const occtdebug::TwoStageFinalResultSyncResult sync =
        occtdebug::TwoStageFinalResultCoordinator::sync(twoStageData, {
            QStringLiteral("failed"),
            QStringLiteral("regression detected"),
            true,
            true,
            afterRows,
            comparison,
            testdiff,
            {
                {QStringLiteral("testgrid"), QStringLiteral("bugs modalg_2"), QStringLiteral("failed"), QStringLiteral("regression"), QStringLiteral("artifacts/testgrid_result.json")},
            },
            timing,
        });

    if (twoStageData.testgridRows.size() != 2
        || twoStageData.manifest.testgridRows.size() != 2
        || twoStageData.diffSummary.isEmpty()
        || !twoStageData.diffSummary.contains(QStringLiteral("testdiff"))
        || twoStageData.verificationItems.size() != 6
        || twoStageData.manifest.verificationItems.size() != 6
        || twoStageData.evidenceItems.size() != 1
        || twoStageData.manifest.evidenceItems.size() != 1
        || sync.evidence.link != QStringLiteral("artifacts/testgrid_two_stage_result.json")
        || !sync.writeEvidenceBundle
        || !sync.writeVerificationReport)
    {
        QTextStream(stderr) << "two-stage final result coordinator did not sync data, evidence, and report triggers\n";
        return 1;
    }

    QLabel diffLabel;
    QTableWidget testgridTable;
    occtdebug::VerificationPanel verificationPanel;
    occtdebug::EvidencePanel evidencePanel;
    const occtdebug::TwoStageFinalResultUiActions uiActions =
        occtdebug::TwoStageFinalResultUiAdapter::apply({
                &diffLabel,
                &testgridTable,
                &verificationPanel,
                &evidencePanel,
            },
            twoStageData,
            sync);
    if (diffLabel.text() != twoStageData.diffSummary
        || testgridTable.rowCount() != twoStageData.testgridRows.size()
        || !uiActions.refreshDiffArtifacts
        || !uiActions.saveCaseManifest
        || !uiActions.writeEvidenceBundle
        || !uiActions.writeVerificationReport)
    {
        QTextStream(stderr) << "two-stage final result UI adapter did not refresh widgets and triggers\n";
        return 1;
    }

    occtdebug::WorkbenchMockData adapterData;
    occtdebug::TestdiffAdapterResultWriterResult adapterWriter;
    adapterWriter.status = QStringLiteral("passed");
    adapterWriter.adapterStatus = QStringLiteral("imported 3 files");
    adapterWriter.diffSummary = QStringLiteral("testdiff changed=1\nimported 3 files");
    const occtdebug::TestdiffAdapterResultSyncResult adapterSync =
        occtdebug::TestdiffAdapterResultCoordinator::sync(adapterData, adapterWriter);
    if (adapterData.diffSummary != adapterWriter.diffSummary
        || adapterData.manifest.diffSummary != adapterWriter.diffSummary
        || adapterData.verificationItems.size() != 1
        || adapterData.verificationItems.first().label != QStringLiteral("testdiff adapter")
        || adapterData.evidenceItems.size() != 1
        || adapterData.manifest.evidenceItems.size() != 1
        || adapterSync.evidence.link != QStringLiteral("artifacts/testdiff_adapter_result.json")
        || !adapterSync.writeEvidenceBundle
        || !adapterSync.writeVerificationReport)
    {
        QTextStream(stderr) << "testdiff adapter coordinator did not sync data, evidence, and report triggers\n";
        return 1;
    }

    QTextStream(stdout) << "WORKBENCH_PRESENTER_SMOKE_OK\n";
    return 0;
}
