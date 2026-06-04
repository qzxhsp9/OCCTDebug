#pragma once

#include "core/patch/PatchReview.h"
#include "core/runner/CommandRunner.h"
#include "workbench/WorkbenchMockData.h"

#include <QMainWindow>

class QLabel;
class QListWidget;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QSplitter;
class QTabWidget;
class QTableWidget;
class QTextEdit;
class QToolButton;
class QTreeWidget;
class QWidget;

class WorkbenchWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit WorkbenchWindow(QWidget* parent = nullptr);

private:
    QWidget* createTitleBar();
    QWidget* createWorkflowToolbar();
    QWidget* createLeftColumn();
    QWidget* createCenterWorkspace();
    QWidget* createRightColumn();
    QWidget* createBottomConsole();

    QWidget* createPanel(const QString& title, QWidget* body);
    QWidget* createCaseListPanel();
    QWidget* createWorkflowPanel();
    QWidget* createKeyInputPanel();
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
    QString currentReproScriptPath() const;
    QString findDrawExecutable() const;
    bool saveCurrentReproScript();
    void runCurrentDrawScript();
    void exportMarkdownReport();
    void updatePatchReviewStatus();
    void applyWorkbenchTheme();
    void populateMockCaseData();

    occtdebug::WorkbenchMockData m_data;
    occtdebug::PatchReviewWorkflow m_patchReview;
    occtdebug::CommandRunner* m_drawRunner = nullptr;
    QLabel* m_caseBadge = nullptr;
    QLabel* m_statusBadge = nullptr;
    QListWidget* m_caseList = nullptr;
    QTreeWidget* m_workflowTree = nullptr;
    QPlainTextEdit* m_reproScriptEdit = nullptr;
    QPlainTextEdit* m_sourceView = nullptr;
    QTextEdit* m_drawConsole = nullptr;
    QTextEdit* m_cmakeConsole = nullptr;
    QTableWidget* m_testgridTable = nullptr;
    QLabel* m_patchReviewStatus = nullptr;
    QProgressBar* m_confidenceBar = nullptr;
};
