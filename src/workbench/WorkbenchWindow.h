#pragma once

#include "core/patch/PatchReview.h"
#include "core/runner/CommandRunner.h"
#include "core/verify/VerificationWorkflow.h"
#include "workbench/WorkbenchMockData.h"

#include <QMainWindow>

class QJsonObject;

namespace occtdebug
{
class OcctViewerWidget;
}

class QLabel;
class QComboBox;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QSplitter;
class QTabWidget;
class QTableWidget;
class QTextEdit;
class QToolButton;
class QWidget;

namespace occtdebug
{
class CasePanel;
class DiffPanel;
class EvidencePanel;
struct ReportRefreshRequest;
class SourcePanel;
class TaskHistoryPanel;
class VerificationPanel;
}

class WorkbenchWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit WorkbenchWindow(QWidget* parent = nullptr);
    explicit WorkbenchWindow(const occtdebug::WorkbenchMockData& initialData, QWidget* parent = nullptr);

private:
    enum class TestgridRunPhase
    {
        Idle,
        DrawGate,
        TestgridCommand,
        TestdiffGate,
        TestdiffCommand,
        TwoStageBeforeGate,
        TwoStageBeforeCommand,
        TwoStagePatchApply,
        TwoStageAfterGate,
        TwoStageAfterCommand,
        TwoStagePatchUndo,
    };

    enum class PatchRunMode
    {
        None,
        Generate,
        Apply,
        Undo,
    };

    QWidget* createTitleBar();
    QWidget* createWorkflowToolbar();
    QWidget* createLeftColumn();
    QWidget* createCenterWorkspace();
    QWidget* createRightColumn();
    QWidget* createBottomConsole();

    QWidget* createPanel(const QString& title, QWidget* body);
    QWidget* createSourceTab();
    QWidget* createReproScriptTab();
    QWidget* createGeometryTab();
    QWidget* createEvidenceTab();
    QWidget* createDiffTab();
    QWidget* createEnvironmentTab();
    QWidget* createDiagnosisPanel();
    QWidget* createPatchPanel();
    QWidget* createPatchReviewPanel();
    QWidget* createVerificationPanel();
    QWidget* createSimilarCasesPanel();

    QLabel* createBadge(const QString& text, const QString& objectName);
    QToolButton* createToolbarButton(const QString& text, const QString& objectName);
    QString runtimeReproDirectory() const;
    QString runtimeReportDirectory() const;
    QString runtimeLogDirectory() const;
    QString runtimeArtifactDirectory() const;
    QString runtimeEnvDirectory() const;
    QString runtimeInputDirectory() const;
    QString runtimeVerificationDirectory() const;
    QString currentReproScriptPath() const;
    QString environmentSnapshotPath() const;
    QString sourceRootDirectory() const;
    QString buildRootDirectory() const;
    QString findDrawExecutable() const;
    bool saveCurrentReproScript();
    bool saveCurrentCaseManifest();
    bool refreshReports(const occtdebug::ReportRefreshRequest& request);
    bool refreshEvidenceBundle();
    bool refreshVerificationReport();
    void createNewCase();
    void openCaseDirectory();
    void loadCaseById(const QString& caseId);
    void refreshCaseList();
    void applyWorkbenchData(occtdebug::WorkbenchMockData nextData);
    void refreshGeometryChecks();
    void refreshDiffArtifactTables();
    bool resolveDiffArtifactPath(const QString& path, const QString& origin, QString* targetPath) const;
    void openDiffArtifactPath(const QString& path, const QString& origin);
    void previewDiffArtifactPath(const QString& path, const QString& origin);
    bool ensureGeometryViewer();
    void importAndLoadGeometryModel();
    void captureGeometryScreenshot();
    void handleViewerGeometryObjectPicked(const QString& objectId, const QString& summary);
    void syncGeometryTopologyStats(const QString& status = QStringLiteral("ok"));
    void recordImportedInputFile(const QString& targetPath, const QString& originalName);
    QString writeTopologySignatureArtifact(const QString& reason,
                                           const QString& selectedObjectId = QString(),
                                           bool registerEvidence = true);
    void exportTopologySignature();
    void generateTopologyCompareArtifact();
    void archiveCrashDump();
    QString focusGeometryObjectFromTopologyCompare(const QJsonObject& compare) const;
    void searchSourceText();
    void openSelectedSourceResult();
    void activateEvidenceRecord(const occtdebug::EvidenceRecord& evidence);
    bool openEvidenceSourceReference(const occtdebug::EvidenceRecord& evidence);
    bool openEvidenceArtifact(const occtdebug::EvidenceRecord& evidence);
    bool selectGeometryEvidence(const occtdebug::EvidenceRecord& evidence);
    QString resolveSourceReferencePath(const QString& fileName) const;
    void refreshSimilarCases(const QString& query = QString());
    QString caseRootDirectory() const;
    QString caseDirectory(const QString& caseId) const;
    QString generateCaseId() const;
    void runCurrentDrawScript();
    void generateCppReproTemplate();
    void runEnvironmentCapture();
    void runTestgridVerification();
    void runTestdiffAdapter();
    void runTwoStageVerification();
    void cancelRunner(occtdebug::CommandRunner* runner, const QString& label, QTextEdit* console);
    void recordTaskStarted(const QString& id,
                           const QString& title,
                           const occtdebug::CommandRequest& request,
                           const QString& artifact = QString(),
                           const QString& stdoutLog = QString(),
                           const QString& stderrLog = QString());
    void recordTaskFinished(const QString& id,
                            const occtdebug::CommandResult& result,
                            const QString& artifact = QString(),
                            const QString& stdoutLog = QString(),
                            const QString& stderrLog = QString(),
                            const QString& note = QString());
    void refreshTaskHistoryPanel();
    void handleTestgridGateFinished(const occtdebug::CommandResult& result);
    void handleTestgridCommandFinished(const occtdebug::CommandResult& result);
    void handleTestdiffGateFinished(const occtdebug::CommandResult& result);
    void handleTestdiffCommandFinished(const occtdebug::CommandResult& result);
    void handleTwoStageGateFinished(const occtdebug::CommandResult& result);
    void handleTwoStageCommandFinished(const occtdebug::CommandResult& result);
    void handleTwoStagePatchFinished(const occtdebug::CommandResult& result);
    void handleTwoStageDecision(const occtdebug::VerificationWorkflowDecision& decision);
    void syncTestgridPlanFromUi();
    bool startConfiguredTestgridCommand(QString* error = nullptr);
    bool startTestgridDrawGate(TestgridRunPhase phase, const QString& label, QString* error = nullptr);
    bool startConfiguredTestgridCommand(TestgridRunPhase phase, const QString& label, QString* error = nullptr);
    bool startConfiguredTestdiffCommand(QString* error = nullptr);
    bool startTwoStagePatchCommand(PatchRunMode mode, QString* error = nullptr);
    void persistTwoStagePhase(const QString& phase,
                              const occtdebug::CommandResult& gateResult,
                              const occtdebug::CommandResult* testgridResult,
                              const QString& note);
    void persistTwoStageWorkflowResult(const QString& finalStatus, const QString& note);
    void exportReproPack();
    void persistDrawRunResult(const occtdebug::CommandResult& result);
    void persistEnvironmentCaptureResult(const occtdebug::CommandResult& result);
    void persistReproPackResult(const occtdebug::CommandResult& result);
    void persistTestgridResult(const occtdebug::CommandResult& gateResult,
                               const occtdebug::CommandResult* testgridResult,
                               const QString& note);
    void persistTestdiffAdapterResult(const occtdebug::CommandResult& result, const QString& note);
    QString environmentSummaryFromSnapshot(const QString& snapshotPath) const;
    void appendEvidenceRecord(const occtdebug::EvidenceRecord& evidence);
    void appendDrawEvidenceFromResult(const occtdebug::CommandResult& result);
    void exportMarkdownReport();
    void exportDiagnosisReport();
    void exportPatchReviewReport();
    void recordPatchReviewState(const QString& note);
    void importPatchCandidateDiff();
    void generatePatchCandidateFromWorktree();
    void savePatchCandidateDiff();
    void exportPatchCandidateDiff();
    void signOffPatchCandidate();
    void applyPatchCandidate();
    void undoPatchCandidate();
    void runPatchCommand(PatchRunMode mode);
    bool runPatchDryRun(PatchRunMode mode, const QString& worktree, const QString& patchPath);
    void persistPatchCandidateGenerationResult(const occtdebug::CommandResult& result);
    void persistPatchCommandResult(const occtdebug::CommandResult& result);
    void updateReproStatusMetric();
    void setVerificationMetric(const QString& labelText, const QString& valueText);
    void updatePatchReviewStatus();
    void applyWorkbenchTheme();
    void populateMockCaseData();

    occtdebug::WorkbenchMockData m_data;
    occtdebug::PatchReviewWorkflow m_patchReview;
    occtdebug::CommandRunner* m_drawRunner = nullptr;
    occtdebug::CommandRunner* m_envRunner = nullptr;
    occtdebug::CommandRunner* m_packRunner = nullptr;
    occtdebug::CommandRunner* m_testgridRunner = nullptr;
    occtdebug::CommandRunner* m_patchRunner = nullptr;
    TestgridRunPhase m_testgridRunPhase = TestgridRunPhase::Idle;
    PatchRunMode m_patchRunMode = PatchRunMode::None;
    occtdebug::CommandResult m_lastTestgridGateResult;
    QString m_lastTestdiffOutputRoot;
    occtdebug::VerificationWorkflow m_twoStageWorkflow;
    QLabel* m_caseBadge = nullptr;
    QLabel* m_statusBadge = nullptr;
    occtdebug::CasePanel* m_casePanel = nullptr;
    QPlainTextEdit* m_reproScriptEdit = nullptr;
    occtdebug::SourcePanel* m_sourcePanel = nullptr;
    occtdebug::DiffPanel* m_diffPanel = nullptr;
    QLabel* m_geometrySummaryLabel = nullptr;
    QWidget* m_geometryViewerHost = nullptr;
    occtdebug::OcctViewerWidget* m_geometryViewer = nullptr;
    QLabel* m_diagnosisLabel = nullptr;
    QPlainTextEdit* m_patchDiffEdit = nullptr;
    QTableWidget* m_geometryTable = nullptr;
    QTextEdit* m_environmentText = nullptr;
    QTextEdit* m_drawConsole = nullptr;
    QTextEdit* m_cmakeConsole = nullptr;
    QTableWidget* m_testgridTable = nullptr;
    QLineEdit* m_testgridGroupEdit = nullptr;
    QLineEdit* m_testgridGridEdit = nullptr;
    QLineEdit* m_testgridCaseEdit = nullptr;
    QSplitter* m_mainSplitter = nullptr;
    QTabWidget* m_workspaceTabs = nullptr;
    QTabWidget* m_bottomTabs = nullptr;
    occtdebug::EvidencePanel* m_evidencePanel = nullptr;
    occtdebug::TaskHistoryPanel* m_taskHistoryPanel = nullptr;
    occtdebug::VerificationPanel* m_verificationPanel = nullptr;
    QListWidget* m_similarCasesList = nullptr;
    QLineEdit* m_similarCaseSearchEdit = nullptr;
    QLabel* m_patchReviewStatus = nullptr;
    QProgressBar* m_confidenceBar = nullptr;
};
