#include "app/MainWindow.h"

#include "core/Logger.h"
#include "io/BRepLoader.h"
#include "io/MarkdownReportExporter.h"
#include "io/ShapeTreeJsonExporter.h"
#include "occt/ShapeInspector.h"
#include "ui/DiagnosticPanel.h"
#include "ui/PropertyPanel.h"
#include "ui/ShapeTreeWidget.h"
#include "ui/ViewerWidget.h"

#include <Standard_Version.hxx>

#include <QAction>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QFile>
#include <QIODevice>
#include <QStatusBar>
#include <QVBoxLayout>

#include <TopoDS_Shape.hxx>

namespace
{
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
    m_diagnosticPanel = new DiagnosticPanel(rightSplitter);
    rightSplitter->addWidget(m_viewer);
    rightSplitter->addWidget(m_diagnosticPanel);
    rightSplitter->setStretchFactor(0, 2);
    rightSplitter->setStretchFactor(1, 1);

    auto* horiz = new QSplitter(Qt::Horizontal, central);
    horiz->addWidget(leftSplitter);
    horiz->addWidget(rightSplitter);
    horiz->setStretchFactor(0, 1);
    horiz->setStretchFactor(1, 2);

    // Native GL child + splitters: geometry can settle after the last resize tick; align OCCT then.
    connect(rightSplitter, &QSplitter::splitterMoved, m_viewer, &ViewerWidget::deferViewportSync);
    connect(horiz, &QSplitter::splitterMoved, m_viewer, &ViewerWidget::deferViewportSync);

    mainLayout->addWidget(horiz);
    setCentralWidget(central);

    auto* fileMenu = menuBar()->addMenu(tr("&File"));
    auto* openAct = fileMenu->addAction(tr("&Open BREP…"));
    openAct->setShortcut(tr("Ctrl+O"));
    connect(openAct, &QAction::triggered, this, &MainWindow::onOpenBrep);

    auto* diagMenu = menuBar()->addMenu(tr("&Diagnostics"));
    auto* runDiagAct = diagMenu->addAction(tr("&Run diagnostics"));
    runDiagAct->setShortcut(tr("F5"));
    connect(runDiagAct, &QAction::triggered, this, &MainWindow::onRunDiagnostics);

    auto* exportMenu = menuBar()->addMenu(tr("&Export"));
    connect(exportMenu->addAction(tr("Diagnostic report (&Markdown)…")), &QAction::triggered, this, &MainWindow::onExportMarkdown);
    connect(exportMenu->addAction(tr("Shape tree (&JSON)…")), &QAction::triggered, this, &MainWindow::onExportShapeJson);

    auto* viewMenu = menuBar()->addMenu(tr("&View"));
    connect(viewMenu->addAction(tr("&Fit all")), &QAction::triggered, m_viewer, &ViewerWidget::fitAll);

    connect(m_shapeTree, &ShapeTreeWidget::shapeSelected, this, &MainWindow::onShapeSelected);
    connect(m_diagnosticPanel, &DiagnosticPanel::findingActivated, this, &MainWindow::onFindingActivated);

    statusBar()->showMessage(tr("OCCT %1 — open a BREP to begin.").arg(QString::fromLatin1(OCC_VERSION_STRING)));

    updateWindowTitle();
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

void MainWindow::onOpenBrep()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        tr("Open BREP"),
        QString(),
        tr("BREP (*.brep *.BREP);;All files (*)"));
    if (path.isEmpty())
    {
        return;
    }

    const BRepLoadResult res = BRepLoader::loadFile(path);
    if (!res.ok)
    {
        Logger::error(res.errorMessage);
        QMessageBox::warning(this, tr("Open failed"), res.errorMessage);
        return;
    }

    ShapeInspector::BuildFromShape(m_document, res.shape);
    m_shapeTree->rebuildFromDocument(m_document);
    m_propertyPanel->showShape(m_document, -1);
    m_diagnosticPanel->setFindings({});
    m_findings.clear();

    m_problem.inputFiles = {path.toStdString()};
    m_viewer->setRootShape(res.shape);
    m_viewer->setHighlightShape(TopoDS_Shape());

    Logger::info(tr("Loaded BREP: %1 (%2 shapes)").arg(path).arg(m_document.Nodes().size()));
    statusBar()->showMessage(tr("Loaded %1").arg(path));
}

void MainWindow::onRunDiagnostics()
{
    if (m_document.RootShape().IsNull())
    {
        QMessageBox::information(this, tr("Diagnostics"), tr("Load a BREP first."));
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
    m_propertyPanel->showShape(m_document, shapeId);
    if (shapeId < 0)
    {
        m_viewer->setHighlightShape(TopoDS_Shape());
        return;
    }
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

void MainWindow::onFindingActivated(int shapeId)
{
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
}
