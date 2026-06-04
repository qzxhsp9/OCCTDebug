#include "workbench/WorkbenchWindow.h"

#include "core/geometry/OcctViewerWidget.h"
#include "core/report/MarkdownReportGenerator.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSplitter>
#include <QStringList>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextCursor>
#include <QTextEdit>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace
{
QString s(const char* text)
{
    return QString::fromUtf8(text);
}

QLabel* label(const QString& text, const QString& objectName = QString())
{
    auto* out = new QLabel(text);
    if (!objectName.isEmpty())
    {
        out->setObjectName(objectName);
    }
    out->setWordWrap(true);
    return out;
}

QTableWidgetItem* item(const QString& text)
{
    auto* out = new QTableWidgetItem(text);
    out->setFlags(out->flags() & ~Qt::ItemIsEditable);
    return out;
}

void setMargins(QLayout* layout, int margin = 10, int spacing = 8)
{
    layout->setContentsMargins(margin, margin, margin, margin);
    layout->setSpacing(spacing);
}
} // namespace

WorkbenchWindow::WorkbenchWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_data(occtdebug::createMockWorkbenchData())
    , m_patchReview(occtdebug::PatchReviewWorkflow::createDefault(m_data.caseId, m_data.patchDiff))
{
    m_drawRunner = new occtdebug::CommandRunner(this);
    connect(m_drawRunner, &occtdebug::CommandRunner::outputReceived, this, [this](const QString& text) {
        if (m_drawConsole != nullptr)
        {
            m_drawConsole->moveCursor(QTextCursor::End);
            m_drawConsole->insertPlainText(text);
        }
    });
    connect(m_drawRunner, &occtdebug::CommandRunner::errorOutputReceived, this, [this](const QString& text) {
        if (m_drawConsole != nullptr)
        {
            m_drawConsole->moveCursor(QTextCursor::End);
            m_drawConsole->insertPlainText(text);
        }
    });
    connect(m_drawRunner, &occtdebug::CommandRunner::finished, this, [this](const occtdebug::CommandResult& result) {
        if (m_drawConsole != nullptr)
        {
            m_drawConsole->append(QString::fromUtf8("\n[DRAW finished] exit=%1 elapsed=%2ms")
                    .arg(result.exitCode)
                    .arg(result.elapsedMs));
        }
    });

    setWindowTitle(s("OCCT 内核专家工作台"));
    setMinimumSize(1440, 900);
    resize(1680, 960);

    auto* root = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(createTitleBar());
    rootLayout->addWidget(createWorkflowToolbar());

    auto* mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->setObjectName(QStringLiteral("MainSplitter"));
    mainSplitter->addWidget(createLeftColumn());
    mainSplitter->addWidget(createCenterWorkspace());
    mainSplitter->addWidget(createRightColumn());
    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setStretchFactor(2, 0);
    mainSplitter->setSizes({300, 980, 380});
    rootLayout->addWidget(mainSplitter, 1);
    rootLayout->addWidget(createBottomConsole());

    setCentralWidget(root);
    applyWorkbenchTheme();
    populateMockCaseData();
}

QWidget* WorkbenchWindow::createTitleBar()
{
    auto* bar = new QFrame;
    bar->setObjectName(QStringLiteral("TitleBar"));
    auto* layout = new QHBoxLayout(bar);
    layout->setContentsMargins(14, 6, 14, 6);
    layout->setSpacing(12);

    auto* icon = label(QStringLiteral("◇"), QStringLiteral("AppIcon"));
    auto* title = label(s("OCCT 内核专家工作台"), QStringLiteral("AppTitle"));
    m_caseBadge = createBadge(QStringLiteral("案例：%1").arg(m_data.caseId), QStringLiteral("Badge"));
    layout->addWidget(icon);
    layout->addWidget(title);
    layout->addSpacing(12);
    layout->addWidget(m_caseBadge);
    layout->addWidget(createBadge(m_data.occtVersion, QStringLiteral("Badge")));
    layout->addWidget(createBadge(m_data.toolchain, QStringLiteral("Badge")));
    layout->addWidget(createBadge(m_data.platform, QStringLiteral("Badge")));
    layout->addStretch(1);
    m_statusBadge = createBadge(QString::fromUtf8("● 状态：%1").arg(m_data.caseStatus), QStringLiteral("SuccessBadge"));
    layout->addWidget(m_statusBadge);
    return bar;
}

QWidget* WorkbenchWindow::createWorkflowToolbar()
{
    auto* bar = new QFrame;
    bar->setObjectName(QStringLiteral("WorkflowToolbar"));
    auto* layout = new QHBoxLayout(bar);
    layout->setContentsMargins(14, 8, 14, 8);
    layout->setSpacing(8);
    layout->addWidget(createToolbarButton(s("① 问题录入"), QStringLiteral("ToolButton")));
    layout->addWidget(createToolbarButton(s("↗ 复现生成"), QStringLiteral("ToolButton")));
    layout->addWidget(createToolbarButton(s("▥ 源码分析"), QStringLiteral("PrimaryToolButton")));
    layout->addWidget(createToolbarButton(s("⚒ 补丁方案"), QStringLiteral("ToolButton")));
    layout->addWidget(createToolbarButton(s("✓ 回归验证"), QStringLiteral("ToolButton")));
    layout->addWidget(createToolbarButton(s("▣ 知识归档"), QStringLiteral("ToolButton")));
    layout->addStretch(1);
    return bar;
}

QWidget* WorkbenchWindow::createLeftColumn()
{
    auto* column = new QWidget;
    column->setObjectName(QStringLiteral("Column"));
    auto* layout = new QVBoxLayout(column);
    setMargins(layout, 10, 8);
    layout->addWidget(createCaseListPanel(), 3);
    layout->addWidget(createWorkflowPanel(), 3);
    layout->addWidget(createKeyInputPanel(), 2);
    return column;
}

QWidget* WorkbenchWindow::createCenterWorkspace()
{
    auto* container = new QWidget;
    container->setObjectName(QStringLiteral("CenterWorkspace"));
    auto* layout = new QVBoxLayout(container);
    setMargins(layout, 0, 0);

    auto* tabs = new QTabWidget;
    tabs->setObjectName(QStringLiteral("WorkspaceTabs"));
    tabs->addTab(createSourceTab(), s("源码"));
    tabs->addTab(createReproScriptTab(), s("复现脚本"));
    tabs->addTab(createGeometryTab(), s("几何视图"));
    tabs->addTab(createEvidenceTab(), s("证据链"));
    tabs->addTab(createDiffTab(), s("差异对比"));
    tabs->addTab(createEnvironmentTab(), s("环境信息"));
    layout->addWidget(tabs);
    return container;
}

QWidget* WorkbenchWindow::createRightColumn()
{
    auto* column = new QWidget;
    column->setObjectName(QStringLiteral("Column"));
    auto* layout = new QVBoxLayout(column);
    setMargins(layout, 10, 8);
    layout->addWidget(createDiagnosisPanel(), 2);
    layout->addWidget(createPatchPanel(), 3);
    layout->addWidget(createPatchReviewPanel(), 1);
    layout->addWidget(createVerificationPanel(), 2);
    layout->addWidget(createSimilarCasesPanel(), 2);
    return column;
}

QWidget* WorkbenchWindow::createBottomConsole()
{
    auto* tabs = new QTabWidget;
    tabs->setObjectName(QStringLiteral("BottomConsole"));
    tabs->setFixedHeight(235);

    m_drawConsole = new QTextEdit;
    m_drawConsole->setReadOnly(true);
    m_drawConsole->setPlainText(m_data.drawConsoleText);

    m_cmakeConsole = new QTextEdit;
    m_cmakeConsole->setReadOnly(true);
    m_cmakeConsole->setPlainText(m_data.cmakeConsoleText);

    m_testgridTable = new QTableWidget(m_data.testgridRows.size(), 5);
    m_testgridTable->setHorizontalHeaderLabels({s("模块"), s("运行"), s("通过"), s("失败"), s("通过率")});
    for (int row = 0; row < m_data.testgridRows.size(); ++row)
    {
        const auto& result = m_data.testgridRows[row];
        m_testgridTable->setItem(row, 0, item(result.module));
        m_testgridTable->setItem(row, 1, item(result.runCount));
        m_testgridTable->setItem(row, 2, item(result.passCount));
        m_testgridTable->setItem(row, 3, item(result.failCount));
        m_testgridTable->setItem(row, 4, item(result.passRate));
    }
    m_testgridTable->horizontalHeader()->setStretchLastSection(true);
    m_testgridTable->verticalHeader()->hide();

    tabs->addTab(m_drawConsole, s("DRAW 控制台"));
    tabs->addTab(m_cmakeConsole, s("PowerShell / CMake"));
    tabs->addTab(m_testgridTable, s("testgrid 结果"));
    return tabs;
}

QWidget* WorkbenchWindow::createPanel(const QString& title, QWidget* body)
{
    auto* frame = new QFrame;
    frame->setObjectName(QStringLiteral("Panel"));
    auto* layout = new QVBoxLayout(frame);
    setMargins(layout, 10, 8);
    auto* heading = label(title, QStringLiteral("PanelTitle"));
    layout->addWidget(heading);
    layout->addWidget(body, 1);
    return frame;
}

QWidget* WorkbenchWindow::createCaseListPanel()
{
    auto* body = new QWidget;
    auto* layout = new QVBoxLayout(body);
    setMargins(layout, 0, 6);
    auto* search = label(s("搜索案例 ID / 关键词"), QStringLiteral("SearchBox"));
    m_caseList = new QListWidget;
    m_caseList->setObjectName(QStringLiteral("CaseList"));
    layout->addWidget(search);
    layout->addWidget(m_caseList, 1);
    return createPanel(s("案例列表"), body);
}

QWidget* WorkbenchWindow::createWorkflowPanel()
{
    m_workflowTree = new QTreeWidget;
    m_workflowTree->setHeaderHidden(true);
    m_workflowTree->setIndentation(12);
    for (const auto& step : m_data.workflowSteps)
    {
        m_workflowTree->addTopLevelItem(
            new QTreeWidgetItem(QStringList{QStringLiteral("%1 %2        %3").arg(step.marker, step.title, step.state)}));
    }
    return createPanel(s("流程状态"), m_workflowTree);
}

QWidget* WorkbenchWindow::createKeyInputPanel()
{
    auto* body = new QWidget;
    auto* grid = new QGridLayout(body);
    setMargins(grid, 0, 6);
    for (int row = 0; row < m_data.keyInputs.size(); ++row)
    {
        const auto& input = m_data.keyInputs[row];
        grid->addWidget(label(input.label, QStringLiteral("MutedText")), row, 0);
        grid->addWidget(label(input.value), row, 1);
    }
    return createPanel(s("关键输入"), body);
}

QWidget* WorkbenchWindow::createSourceTab()
{
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setObjectName(QStringLiteral("SourceSplitter"));

    m_sourceView = new QPlainTextEdit;
    m_sourceView->setReadOnly(true);
    m_sourceView->setPlainText(m_data.sourceText);

    auto* side = new QWidget;
    auto* sideLayout = new QVBoxLayout(side);
    setMargins(sideLayout, 0, 0);
    sideLayout->addWidget(createGeometryTab(), 3);
    sideLayout->addWidget(label(m_data.evidenceSummary, QStringLiteral("EvidenceCard")), 2);

    splitter->addWidget(createPanel(s("源码"), m_sourceView));
    splitter->addWidget(side);
    splitter->setSizes({560, 480});
    return splitter;
}

QWidget* WorkbenchWindow::createReproScriptTab()
{
    auto* body = new QWidget;
    auto* layout = new QVBoxLayout(body);
    setMargins(layout, 0, 6);

    m_reproScriptEdit = new QPlainTextEdit;
    m_reproScriptEdit->setPlainText(m_data.reproScript);
    layout->addWidget(m_reproScriptEdit, 3);

    auto* actions = new QWidget;
    auto* actionLayout = new QHBoxLayout(actions);
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(8);
    auto* saveButton = new QPushButton(s("保存 repro.tcl"));
    auto* runButton = new QPushButton(s("运行 DRAW"));
    connect(saveButton, &QPushButton::clicked, this, [this]() {
        saveCurrentReproScript();
    });
    connect(runButton, &QPushButton::clicked, this, [this]() {
        runCurrentDrawScript();
    });
    actionLayout->addWidget(saveButton);
    actionLayout->addWidget(runButton);
    actionLayout->addStretch(1);
    layout->addWidget(actions);
    layout->addWidget(label(m_data.evidenceSummary, QStringLiteral("EvidenceCard")), 1);
    return createPanel(s("复现脚本"), body);
}

QWidget* WorkbenchWindow::createGeometryTab()
{
    auto* body = new QWidget;
    auto* layout = new QVBoxLayout(body);
    setMargins(layout, 0, 6);
    auto* viewport = new occtdebug::OcctViewerWidget;
    viewport->setObjectName(QStringLiteral("GeometryViewport"));
    layout->addWidget(viewport, 1);
    layout->addWidget(label(m_data.geometrySummary, QStringLiteral("EvidenceCard")));

    auto* table = new QTableWidget(m_data.geometryChecks.size(), 3);
    table->setHorizontalHeaderLabels({s("检查项"), s("状态"), s("备注")});
    for (int row = 0; row < m_data.geometryChecks.size(); ++row)
    {
        const auto& check = m_data.geometryChecks[row];
        table->setItem(row, 0, item(check.name));
        table->setItem(row, 1, item(check.status));
        table->setItem(row, 2, item(check.note));
    }
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->hide();
    layout->addWidget(table);
    return createPanel(s("几何视图 / 几何检查"), body);
}

QWidget* WorkbenchWindow::createDiffTab()
{
    return createPanel(s("差异对比"), label(m_data.diffSummary));
}

QWidget* WorkbenchWindow::createEvidenceTab()
{
    auto* body = new QWidget;
    auto* layout = new QVBoxLayout(body);
    setMargins(layout, 0, 6);
    layout->addWidget(label(m_data.evidenceSummary, QStringLiteral("EvidenceCard")));

    auto* table = new QTableWidget(m_data.evidenceItems.size(), 4);
    table->setHorizontalHeaderLabels({s("类型"), s("标题"), s("摘要"), s("链接")});
    for (int row = 0; row < m_data.evidenceItems.size(); ++row)
    {
        const auto& evidence = m_data.evidenceItems[row];
        table->setItem(row, 0, item(evidence.type));
        table->setItem(row, 1, item(evidence.title));
        table->setItem(row, 2, item(evidence.summary));
        table->setItem(row, 3, item(evidence.link));
    }
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->hide();
    layout->addWidget(table, 1);
    return createPanel(s("证据链"), body);
}

QWidget* WorkbenchWindow::createEnvironmentTab()
{
    return createPanel(s("环境信息"), label(m_data.environmentSummary));
}

QWidget* WorkbenchWindow::createDiagnosisPanel()
{
    auto* body = new QWidget;
    auto* layout = new QVBoxLayout(body);
    setMargins(layout, 0, 6);
    layout->addWidget(label(m_data.diagnosis));
    m_confidenceBar = new QProgressBar;
    m_confidenceBar->setRange(0, 100);
    m_confidenceBar->setValue(m_data.diagnosisConfidence);
    m_confidenceBar->setFormat(s("置信度：高 (%p%)"));
    layout->addWidget(m_confidenceBar);
    return createPanel(s("诊断结论"), body);
}

QWidget* WorkbenchWindow::createPatchPanel()
{
    auto* diff = new QPlainTextEdit;
    diff->setReadOnly(true);
    diff->setPlainText(m_data.patchDiff);
    return createPanel(s("候选补丁"), diff);
}

QWidget* WorkbenchWindow::createVerificationPanel()
{
    auto* body = new QWidget;
    auto* layout = new QVBoxLayout(body);
    setMargins(layout, 0, 6);

    auto* gridHost = new QWidget;
    auto* grid = new QGridLayout(gridHost);
    setMargins(grid, 0, 6);
    for (int row = 0; row < m_data.verificationItems.size(); ++row)
    {
        const auto& metric = m_data.verificationItems[row];
        grid->addWidget(label(metric.label, QStringLiteral("MutedText")), row, 0);
        grid->addWidget(label(metric.value), row, 1);
    }
    layout->addWidget(gridHost);

    auto* exportButton = new QPushButton(s("导出 Markdown 报告"));
    connect(exportButton, &QPushButton::clicked, this, [this]() {
        exportMarkdownReport();
    });
    layout->addWidget(exportButton);
    return createPanel(s("验证结果"), body);
}

QWidget* WorkbenchWindow::createPatchReviewPanel()
{
    auto* body = new QWidget;
    auto* layout = new QVBoxLayout(body);
    setMargins(layout, 0, 6);

    m_patchReviewStatus = label(QString(), QStringLiteral("EvidenceCard"));
    updatePatchReviewStatus();
    layout->addWidget(m_patchReviewStatus);

    auto* actions = new QWidget;
    auto* actionLayout = new QHBoxLayout(actions);
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(8);

    auto* reviewButton = new QPushButton(QStringLiteral("Review"));
    auto* approveButton = new QPushButton(QStringLiteral("Approve"));
    auto* rejectButton = new QPushButton(QStringLiteral("Reject"));
    connect(reviewButton, &QPushButton::clicked, this, [this]() {
        m_patchReview.markNeedsReview(QStringLiteral("Evidence is linked; ready for maintainer review."));
        updatePatchReviewStatus();
    });
    connect(approveButton, &QPushButton::clicked, this, [this]() {
        m_patchReview.approve(QStringLiteral("Reviewer accepted the candidate direction."));
        updatePatchReviewStatus();
    });
    connect(rejectButton, &QPushButton::clicked, this, [this]() {
        m_patchReview.reject(QStringLiteral("Reviewer rejected the current patch candidate."));
        updatePatchReviewStatus();
    });

    actionLayout->addWidget(reviewButton);
    actionLayout->addWidget(approveButton);
    actionLayout->addWidget(rejectButton);
    layout->addWidget(actions);

    return createPanel(QStringLiteral("Patch Review"), body);
}

QWidget* WorkbenchWindow::createSimilarCasesPanel()
{
    auto* body = new QListWidget;
    for (const auto& similarCase : m_data.similarCases)
    {
        body->addItem(QStringLiteral("%1  %2   相似度 %3").arg(similarCase.id, similarCase.title, similarCase.score));
    }
    return createPanel(s("相似案例 / Issues"), body);
}

QLabel* WorkbenchWindow::createBadge(const QString& text, const QString& objectName)
{
    auto* out = label(text, objectName);
    out->setAlignment(Qt::AlignCenter);
    out->setMinimumHeight(26);
    return out;
}

QToolButton* WorkbenchWindow::createToolbarButton(const QString& text, const QString& objectName)
{
    auto* button = new QToolButton;
    button->setText(text);
    button->setObjectName(objectName);
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button->setMinimumHeight(34);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

void WorkbenchWindow::populateMockCaseData()
{
    for (const auto& caseSummary : m_data.cases)
    {
        m_caseList->addItem(QStringLiteral("%1    %2\n%3\n创建：%4")
                .arg(caseSummary.id, caseSummary.status, caseSummary.title, caseSummary.createdAt));
    }
    m_caseList->setCurrentRow(0);
}

QString WorkbenchWindow::runtimeReproDirectory() const
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("runtime/%1/repro").arg(m_data.caseId));
}

QString WorkbenchWindow::runtimeReportDirectory() const
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("runtime/%1/report").arg(m_data.caseId));
}

QString WorkbenchWindow::currentReproScriptPath() const
{
    return QDir(runtimeReproDirectory()).filePath(QStringLiteral("repro.tcl"));
}

QString WorkbenchWindow::findDrawExecutable() const
{
    QStringList candidates;
#ifdef OCCTDEBUG_SOURCE_DIR
    const QString sourceRoot = QStringLiteral(OCCTDEBUG_SOURCE_DIR);
    candidates << QDir(sourceRoot).filePath(QStringLiteral("depends/occt/lib/Debug/bind/DRAWEXE.exe"));
    candidates << QDir(sourceRoot).filePath(QStringLiteral("depends/occt/lib/Release/bin/DRAWEXE.exe"));
    candidates << QDir(sourceRoot).filePath(QStringLiteral("depends/occt/lib/RelWithDebInfo/bini/DRAWEXE.exe"));
#endif
    for (const QString& candidate : candidates)
    {
        if (QFileInfo::exists(candidate))
        {
            return QFileInfo(candidate).absoluteFilePath();
        }
    }
    return QString();
}

bool WorkbenchWindow::saveCurrentReproScript()
{
    if (m_reproScriptEdit == nullptr)
    {
        return false;
    }

    QDir dir;
    if (!dir.mkpath(runtimeReproDirectory()))
    {
        if (m_drawConsole != nullptr)
        {
            m_drawConsole->append(QStringLiteral("[repro] failed to create runtime directory"));
        }
        return false;
    }

    QFile file(currentReproScriptPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (m_drawConsole != nullptr)
        {
            m_drawConsole->append(QStringLiteral("[repro] failed to write %1: %2").arg(file.fileName(), file.errorString()));
        }
        return false;
    }

    file.write(m_reproScriptEdit->toPlainText().toUtf8());
    if (m_drawConsole != nullptr)
    {
        m_drawConsole->append(QStringLiteral("[repro] saved %1").arg(QDir::toNativeSeparators(file.fileName())));
    }
    return true;
}

void WorkbenchWindow::runCurrentDrawScript()
{
    if (m_drawRunner == nullptr || m_drawRunner->isRunning())
    {
        if (m_drawConsole != nullptr)
        {
            m_drawConsole->append(QStringLiteral("[DRAW] runner is busy"));
        }
        return;
    }

    if (!saveCurrentReproScript())
    {
        return;
    }

    const QString drawExe = findDrawExecutable();
    if (drawExe.isEmpty())
    {
        if (m_drawConsole != nullptr)
        {
            m_drawConsole->append(QStringLiteral("[DRAW] DRAWEXE.exe was not found under depends/occt"));
        }
        return;
    }

    const QString scriptPath = currentReproScriptPath();
    const QString command = QStringLiteral("\"%1\" < \"%2\"")
            .arg(QDir::toNativeSeparators(drawExe), QDir::toNativeSeparators(scriptPath));

    occtdebug::CommandRequest request;
    request.program = QStringLiteral("cmd.exe");
    request.arguments = {QStringLiteral("/c"), command};
    request.workingDirectory = runtimeReproDirectory();
    request.environment = QProcessEnvironment::systemEnvironment();
    request.environment.insert(QStringLiteral("CASROOT"), QFileInfo(drawExe).absolutePath());
    request.environment.insert(QStringLiteral("PATH"),
        QFileInfo(drawExe).absolutePath() + QStringLiteral(";") + request.environment.value(QStringLiteral("PATH")));

    if (m_drawConsole != nullptr)
    {
        m_drawConsole->append(QStringLiteral("[DRAW] %1").arg(command));
    }

    QString error;
    if (!m_drawRunner->start(request, &error) && m_drawConsole != nullptr)
    {
        m_drawConsole->append(QStringLiteral("[DRAW] failed to start: %1").arg(error));
    }
}

void WorkbenchWindow::exportMarkdownReport()
{
    const QString reportPath = QDir(runtimeReportDirectory()).filePath(QStringLiteral("repro_report.md"));
    QString error;
    if (!occtdebug::MarkdownReportGenerator::writeReproReport(m_data.manifest, reportPath, &error))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[report] failed: %1").arg(error));
        }
        return;
    }

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[report] generated %1").arg(QDir::toNativeSeparators(reportPath)));
    }
}

void WorkbenchWindow::updatePatchReviewStatus()
{
    if (m_patchReviewStatus == nullptr)
    {
        return;
    }

    QStringList lines;
    lines << QStringLiteral("Review status: %1").arg(m_patchReview.statusText());
    for (const occtdebug::PatchReviewStep& step : m_patchReview.steps())
    {
        lines << QStringLiteral("- %1: %2").arg(step.title, step.state);
    }
    m_patchReviewStatus->setText(lines.join(QLatin1Char('\n')));
}

void WorkbenchWindow::applyWorkbenchTheme()
{
    qApp->setStyleSheet(QStringLiteral(R"css(
        QMainWindow, QWidget {
            background: #07111c;
            color: #d8e5f2;
            font-family: "Microsoft YaHei UI", "Segoe UI";
            font-size: 13px;
        }
        #TitleBar {
            background: #050c14;
            border-bottom: 1px solid #142235;
        }
        #WorkflowToolbar {
            background: #0a1724;
            border-bottom: 1px solid #17283b;
        }
        #AppIcon {
            color: #55a7ff;
            font-size: 22px;
            font-weight: 700;
        }
        #AppTitle {
            font-size: 17px;
            font-weight: 700;
            color: #f2f7ff;
        }
        #Badge, #SuccessBadge {
            border: 1px solid #24374d;
            border-radius: 4px;
            padding: 2px 10px;
            background: #0d1a29;
            color: #aebbd0;
        }
        #SuccessBadge {
            color: #65df8b;
            border-color: #23553c;
            background: #0b2118;
        }
        #ToolButton, #PrimaryToolButton {
            border: 1px solid #26394f;
            border-radius: 5px;
            padding: 6px 14px;
            background: #101d2b;
            color: #c7d5e7;
        }
        #PrimaryToolButton {
            background: #123968;
            border-color: #2862a8;
            color: #eaf4ff;
        }
        #ToolButton:hover, #PrimaryToolButton:hover {
            background: #17304b;
        }
        #Panel {
            background: #0a1623;
            border: 1px solid #1a2b40;
            border-radius: 6px;
        }
        #PanelTitle {
            color: #b9d9ff;
            font-weight: 700;
        }
        #SearchBox, #EvidenceCard {
            border: 1px solid #20354d;
            border-radius: 4px;
            padding: 7px 9px;
            background: #081320;
            color: #8fa3ba;
        }
        #GeometryViewport {
            border: 1px solid #213954;
            border-radius: 4px;
            background: #06101b;
            color: #9db4c9;
            min-height: 230px;
        }
        #MutedText {
            color: #7f91a8;
        }
        QTabWidget::pane {
            border: 1px solid #182a3f;
            background: #081421;
        }
        QTabBar::tab {
            background: #0c1927;
            border: 1px solid #182a3f;
            padding: 7px 18px;
            color: #8fa3ba;
        }
        QTabBar::tab:selected {
            background: #12365b;
            color: #e5f1ff;
            border-bottom-color: #2b83d8;
        }
        QListWidget, QTreeWidget, QTableWidget, QPlainTextEdit, QTextEdit {
            background: #07121e;
            border: 1px solid #1a2b40;
            border-radius: 4px;
            selection-background-color: #154d86;
            selection-color: #ffffff;
            color: #d1dfef;
        }
        QPlainTextEdit, QTextEdit {
            font-family: "Cascadia Code", Consolas, monospace;
            font-size: 13px;
        }
        QHeaderView::section {
            background: #0e1d2c;
            border: 0;
            border-right: 1px solid #1a2b40;
            padding: 5px;
            color: #9db4c9;
        }
        QProgressBar {
            background: #132031;
            border: 1px solid #26394f;
            border-radius: 4px;
            text-align: center;
            color: #d8e5f2;
        }
        QProgressBar::chunk {
            background: #57bf73;
            border-radius: 3px;
        }
        QSplitter::handle {
            background: #07111c;
        }
    )css"));
}
