#include "app/MainWindow.h"

#include "core/DebugSession.h"
#include "core/Logger.h"
#include "io/BRepLoader.h"
#include "io/MarkdownReportExporter.h"
#include "io/ReproPackageExporter.h"
#include "io/SessionSerializer.h"
#include "io/ShapeTreeJsonExporter.h"
#include "occt/ShapeInspector.h"
#include "ui/DiagnosticPanel.h"
#include "ui/PropertyPanel.h"
#include "ui/ShapeTreeWidget.h"
#include "ui/TopologyDetailPanel.h"
#include "ui/ViewerWidget.h"

#include <Standard_Version.hxx>

#include <QAction>
#include <QDateTime>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QFile>
#include <QIODevice>
#include <QApplication>
#include <QGuiApplication>
#include <QStatusBar>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QEvent>

#include <TopoDS_Shape.hxx>

namespace
{
QString inputTypeFromFilePath(const QString& filePath)
{
    const QString ext = QFileInfo(filePath).suffix().toLower();
    if (ext == QStringLiteral("stp") || ext == QStringLiteral("step"))
    {
        return QStringLiteral("step");
    }
    return QStringLiteral("brep");
}

void fillBuildMetadata(ProblemContext& ctx)
{
    ctx.occtVersion = OCC_VERSION_STRING;
#ifdef _MSC_VER
    ctx.compiler = std::string("MSVC ") + std::to_string(_MSC_VER);
#else
    ctx.compiler = "non-MSVC";
#endif
#if defined(_DEBUG) || !defined(NDEBUG)
    ctx.buildType = "Debug";
#else
    ctx.buildType = "Release";
#endif
}
} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setMinimumSize(1024, 700);
    applyProblemDefaults();

    auto* central = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(central);

    auto* leftSplitter = new QSplitter(Qt::Vertical, central);
    m_shapeTree = new ShapeTreeWidget(leftSplitter);
    m_propertyPanel = new PropertyPanel(leftSplitter);
    leftSplitter->addWidget(m_shapeTree);
    leftSplitter->addWidget(m_propertyPanel);
    leftSplitter->setStretchFactor(0, 2);
    leftSplitter->setStretchFactor(1, 1);

    auto* rightSplitter = new QSplitter(Qt::Vertical, central);
    m_viewer = new ViewerWidget(rightSplitter);
    m_topologyPanel = new TopologyDetailPanel(rightSplitter);
    rightSplitter->addWidget(m_viewer);
    rightSplitter->addWidget(m_topologyPanel);
    rightSplitter->setStretchFactor(0, 2);
    rightSplitter->setStretchFactor(1, 1);
    rightSplitter->setSizes({480, 240});

    auto* horiz = new QSplitter(Qt::Horizontal, central);
    horiz->addWidget(leftSplitter);
    horiz->addWidget(rightSplitter);
    horiz->setStretchFactor(0, 1);
    horiz->setStretchFactor(1, 2);

    connect(rightSplitter, &QSplitter::splitterMoved, m_viewer, &ViewerWidget::deferViewportSync);
    connect(horiz, &QSplitter::splitterMoved, m_viewer, &ViewerWidget::deferViewportSync);

    mainLayout->addWidget(horiz);
    setCentralWidget(central);

    m_diagnosticDock = new QDockWidget(tr("Diagnostic log"), this);
    m_diagnosticDock->setObjectName(QStringLiteral("DiagnosticDock"));
    m_diagnosticPanel = new DiagnosticPanel(m_diagnosticDock);
    m_diagnosticDock->setWidget(m_diagnosticPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_diagnosticDock);

    auto* fileMenu = menuBar()->addMenu(tr("&File"));
    auto* openAct = fileMenu->addAction(tr("Open &model…"));
    openAct->setShortcut(tr("Ctrl+O"));
    connect(openAct, &QAction::triggered, this, &MainWindow::onOpenBrep);

    auto* openSessionAct = fileMenu->addAction(tr("Open &session…"));
    openSessionAct->setShortcut(tr("Ctrl+Shift+O"));
    connect(openSessionAct, &QAction::triggered, this, &MainWindow::onOpenSession);

    auto* saveSessionAct = fileMenu->addAction(tr("&Save session…"));
    saveSessionAct->setShortcut(tr("Ctrl+Shift+S"));
    connect(saveSessionAct, &QAction::triggered, this, &MainWindow::onSaveSession);

    auto* diagMenu = menuBar()->addMenu(tr("&Diagnostics"));
    auto* runDiagAct = diagMenu->addAction(tr("&Run diagnostics"));
    runDiagAct->setShortcut(tr("F5"));
    connect(runDiagAct, &QAction::triggered, this, &MainWindow::onRunDiagnostics);

    auto* exportMenu = menuBar()->addMenu(tr("&Export"));
    connect(exportMenu->addAction(tr("Diagnostic report (&Markdown)…")), &QAction::triggered, this, &MainWindow::onExportMarkdown);
    connect(exportMenu->addAction(tr("Shape tree (&JSON)…")), &QAction::triggered, this, &MainWindow::onExportShapeJson);
    connect(exportMenu->addAction(tr("Minimal &repro folder…")), &QAction::triggered, this, &MainWindow::onExportMinimalRepro);

    auto* viewMenu = menuBar()->addMenu(tr("&View"));
    connect(viewMenu->addAction(tr("&Fit all")), &QAction::triggered, m_viewer, &ViewerWidget::fitAll);
    auto* diagDockAct = viewMenu->addAction(tr("&Diagnostic log dock"));
    diagDockAct->setCheckable(true);
    diagDockAct->setChecked(true);
    connect(diagDockAct, &QAction::toggled, m_diagnosticDock, &QWidget::setVisible);
    auto* bboxAct = viewMenu->addAction(tr("Show &bounding box"));
    bboxAct->setCheckable(true);
    bboxAct->setChecked(m_viewer->showBoundingBox());
    connect(bboxAct, &QAction::toggled, m_viewer, &ViewerWidget::setShowBoundingBox);

    auto* helpMenu = menuBar()->addMenu(tr("&Help"));
    connect(helpMenu->addAction(tr("&Mouse controls")), &QAction::triggered, this, [this]() {
        QMessageBox::information(
            this,
            tr("Mouse controls"),
            tr("3D view:\n"
               "- Left drag: rotate\n"
               "- Middle drag: pan\n"
               "- Wheel: zoom\n"
               "- Double-click: fit all\n\n"
               "Topology detail:\n"
               "- Middle drag: pan\n"
               "- Wheel: zoom\n"
               "- Double-click: fit"));
    });

    connect(m_shapeTree, &ShapeTreeWidget::shapeSelected, this, &MainWindow::onShapeSelected);
    connect(m_diagnosticPanel, &DiagnosticPanel::findingActivated, this, &MainWindow::onFindingActivated);

    // Qt6: QApplication::focusChanged was removed; use QGuiApplication::focusObjectChanged.
    connect(qApp, &QGuiApplication::focusObjectChanged, this, [this](QObject* focusObject) {
        if (m_viewer == nullptr)
        {
            return;
        }
        auto* newWidget = qobject_cast<QWidget*>(focusObject);
        const auto underViewer = [this](QWidget* w) -> bool {
            return w != nullptr && (w == m_viewer || m_viewer->isAncestorOf(w));
        };
        const bool oldUnderViewer = underViewer(m_prevFocusWidget);
        const bool newUnderViewer = underViewer(newWidget);
        if (oldUnderViewer || newUnderViewer)
        {
            m_viewer->refreshPresentation();
        }
        m_prevFocusWidget = newWidget;
    });

    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (m_viewer != nullptr && state == Qt::ApplicationActive)
        {
            m_viewer->refreshPresentation();
        }
    });

    statusBar()->showMessage(tr("OCCT %1 — open a BREP or STEP model to begin.")
                                 .arg(QString::fromLatin1(OCC_VERSION_STRING)));

    updateWindowTitle();
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    if (m_viewer != nullptr)
    {
        m_viewer->refreshPresentation();
    }
    // First layout pass often runs after this event; refresh again on the next tick.
    QTimer::singleShot(0, this, [this]() {
        if (m_viewer != nullptr)
        {
            m_viewer->refreshPresentation();
        }
    });
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowActivate && isActiveWindow() && m_viewer != nullptr)
    {
        m_viewer->refreshPresentation();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::applyProblemDefaults()
{
    m_problem.title.clear();
    m_problem.category = ProblemCategory::Unknown;
    m_problem.description.clear();
    fillBuildMetadata(m_problem);
}

void MainWindow::updateWindowTitle()
{
    setWindowTitle(tr("OCCTDebug"));
}

bool MainWindow::openBrepPath(const QString& path, QString* errorOut)
{
    const BRepLoadResult res = BRepLoader::loadFile(path);
    if (!res.ok)
    {
        if (errorOut != nullptr)
        {
            *errorOut = res.errorMessage;
        }
        return false;
    }

    ShapeInspector::BuildFromShape(m_document, res.shape);
    m_shapeTree->rebuildFromDocument(m_document);
    m_propertyPanel->showShape(m_document, -1);
    m_diagnosticPanel->setFindings({});
    m_findings.clear();

    m_problem.inputFiles = {path.toStdString()};
    m_viewer->setRootShape(res.shape);
    m_viewer->setHighlightShape(TopoDS_Shape());
    m_selectedShapeId = -1;
    if (m_topologyPanel != nullptr)
    {
        m_topologyPanel->inspect(m_document, -1);
    }
    m_viewer->refreshPresentation();
    return true;
}

void MainWindow::onOpenBrep()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        tr("Open model"),
        QString(),
        tr("Geometry (*.brep *.BREP *.stp *.STP *.step *.STEP);;BREP (*.brep *.BREP);;STEP "
           "(*.stp *.STP *.step *.STEP);;All files (*)"));
    if (path.isEmpty())
    {
        return;
    }

    QString err;
    if (!openBrepPath(path, &err))
    {
        Logger::error(err);
        QMessageBox::warning(this, tr("Open failed"), err);
        return;
    }

    m_sessionFilePath.clear();

    Logger::info(tr("Loaded model: %1 (%2 shapes)").arg(path).arg(m_document.Nodes().size()));
    statusBar()->showMessage(tr("Loaded %1").arg(path));
}

void MainWindow::onSaveSession()
{
    if (m_document.RootShape().IsNull())
    {
        QMessageBox::information(this, tr("Session"), tr("Load a model before saving a session."));
        return;
    }

    QString path = QFileDialog::getSaveFileName(
        this,
        tr("Save session"),
        m_sessionFilePath.isEmpty() ? QStringLiteral("debug.occtdbg") : m_sessionFilePath,
        tr("OCCTDebug session (*.occtdbg);;All files (*)"));
    if (path.isEmpty())
    {
        return;
    }

    DebugSession session;
    session.version = DebugSession::kCurrentVersion;
    session.createdAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs).toStdString();
    session.problem = m_problem;
    session.diagnostics = m_findings;
    session.selectedShapeId = m_selectedShapeId;

    for (const std::string& p : m_problem.inputFiles)
    {
        SessionInput in;
        const QString absPath = QString::fromStdString(p);
        in.path = SessionSerializer::toStoredPath(absPath, path).toStdString();
        in.type = inputTypeFromFilePath(absPath).toStdString();
        in.role = "primary";
        session.inputs.push_back(std::move(in));
    }

    QString err;
    if (!SessionSerializer::save(path, session, &err))
    {
        QMessageBox::warning(this, tr("Session"), err);
        return;
    }

    m_sessionFilePath = path;
    Logger::info(tr("Saved session: %1").arg(path));
    statusBar()->showMessage(tr("Saved session %1").arg(path));
}

void MainWindow::onOpenSession()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        tr("Open session"),
        QString(),
        tr("OCCTDebug session (*.occtdbg);;All files (*)"));
    if (path.isEmpty())
    {
        return;
    }

    DebugSession session;
    QString err;
    if (!SessionSerializer::load(path, &session, &err))
    {
        QMessageBox::warning(this, tr("Session"), err);
        return;
    }

    QString modelPath;
    for (const SessionInput& in : session.inputs)
    {
        const QString rawPath = QString::fromStdString(in.path);
        const QString resolved = SessionSerializer::resolveInputPath(rawPath, path);
        if (QFileInfo::exists(resolved))
        {
            modelPath = resolved;
            break;
        }
    }
    if (modelPath.isEmpty())
    {
        for (const std::string& fp : session.problem.inputFiles)
        {
            const QString resolved =
                SessionSerializer::resolveInputPath(QString::fromStdString(fp), path);
            if (QFileInfo::exists(resolved))
            {
                modelPath = resolved;
                break;
            }
        }
    }

    if (modelPath.isEmpty() || !QFileInfo::exists(modelPath))
    {
        QMessageBox::warning(
            this,
            tr("Session"),
            tr("Could not find the model file for this session. Check paths relative to the session file."));
        return;
    }

    if (!openBrepPath(modelPath, &err))
    {
        QMessageBox::warning(this, tr("Open failed"), err);
        return;
    }

    m_problem = session.problem;
    m_problem.inputFiles = {modelPath.toStdString()};
    m_findings = std::move(session.diagnostics);
    m_diagnosticPanel->setFindings(m_findings);

    m_sessionFilePath = path;

    if (session.selectedShapeId >= 0 && m_document.FindNode(session.selectedShapeId) != nullptr)
    {
        m_shapeTree->selectShapeId(session.selectedShapeId);
    }

    Logger::info(tr("Opened session: %1").arg(path));
    statusBar()->showMessage(tr("Session %1").arg(path));
}

void MainWindow::onRunDiagnostics()
{
    if (m_document.RootShape().IsNull())
    {
        QMessageBox::information(this, tr("Diagnostics"), tr("Load a model first."));
        return;
    }
    m_findings = m_engine.diagnose(m_problem, m_document);
    m_diagnosticPanel->setFindings(m_findings);
    Logger::info(tr("Diagnostics finished: %1 finding(s)").arg(m_findings.size()));
    statusBar()->showMessage(tr("%1 finding(s)").arg(m_findings.size()));
}

void MainWindow::onExportMarkdown()
{
    if (m_document.RootShape().IsNull())
    {
        QMessageBox::information(this, tr("Export"), tr("Nothing to export; load a model first."));
        return;
    }
    const QString path = QFileDialog::getSaveFileName(
        this,
        tr("Export Markdown"),
        QStringLiteral("occtdebug-report.md"),
        tr("Markdown (*.md)"));
    if (path.isEmpty())
    {
        return;
    }
    const QString md = MarkdownReportExporter::exportReport(m_problem, m_document, m_findings);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, tr("Export failed"), f.errorString());
        return;
    }
    f.write(md.toUtf8());
    Logger::info(tr("Exported Markdown: %1").arg(path));
}

void MainWindow::onExportMinimalRepro()
{
    if (m_document.RootShape().IsNull() || m_problem.inputFiles.empty())
    {
        QMessageBox::information(this, tr("Export"), tr("Load a model first."));
        return;
    }

    const QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Select folder for minimal repro package"),
        QFileInfo(QString::fromStdString(m_problem.inputFiles.front())).absolutePath());
    if (dir.isEmpty())
    {
        return;
    }

    const QString brep = QFileInfo(QString::fromStdString(m_problem.inputFiles.front())).absoluteFilePath();
    QString err;
    if (!ReproPackageExporter::exportMinimalPackage(dir, m_problem, brep, m_findings, &err))
    {
        QMessageBox::warning(this, tr("Export"), err);
        return;
    }
    Logger::info(tr("Exported minimal repro to %1").arg(dir));
    statusBar()->showMessage(tr("Minimal repro exported to %1").arg(dir));
}

void MainWindow::onExportShapeJson()
{
    if (m_document.RootShape().IsNull())
    {
        QMessageBox::information(this, tr("Export"), tr("Nothing to export; load a model first."));
        return;
    }
    const QString path = QFileDialog::getSaveFileName(
        this,
        tr("Export shape tree JSON"),
        QStringLiteral("shape-tree.json"),
        tr("JSON (*.json)"));
    if (path.isEmpty())
    {
        return;
    }
    const QByteArray json = ShapeTreeJsonExporter::exportDocument(m_document);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, tr("Export failed"), f.errorString());
        return;
    }
    f.write(json);
    Logger::info(tr("Exported JSON: %1").arg(path));
}

void MainWindow::onShapeSelected(int shapeId)
{
    m_selectedShapeId = shapeId;
    m_propertyPanel->showShape(m_document, shapeId);
    if (shapeId < 0)
    {
        m_viewer->setHighlightShape(TopoDS_Shape());
    }
    else
    {
        const ShapeNode* node = m_document.FindNode(shapeId);
        if (node && !node->shape.IsNull())
        {
            m_viewer->setHighlightShape(node->shape);
        }
        else
        {
            m_viewer->setHighlightShape(TopoDS_Shape());
        }
    }
    if (m_topologyPanel != nullptr)
    {
        m_topologyPanel->inspect(m_document, shapeId);
    }
}

void MainWindow::onFindingActivated(int shapeId)
{
    m_selectedShapeId = shapeId;
    if (shapeId >= 0)
    {
        m_shapeTree->selectShapeId(shapeId);
        m_propertyPanel->showShape(m_document, shapeId);
        const ShapeNode* node = m_document.FindNode(shapeId);
        if (node && !node->shape.IsNull())
        {
            m_viewer->setHighlightShape(node->shape);
        }
    }
    if (m_topologyPanel != nullptr)
    {
        m_topologyPanel->inspect(m_document, shapeId);
    }
}
