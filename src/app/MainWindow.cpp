#include "app/MainWindow.h"

#include "analysis/ProblemDocumentImporter.h"
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

#include <BRep_Builder.hxx>
#include <QAction>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDirIterator>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QSplitter>
#include <QFile>
#include <QIODevice>
#include <QApplication>
#include <QAbstractItemView>
#include <QComboBox>
#include <QGuiApplication>
#include <QLineEdit>
#include <QStatusBar>
#include <QShowEvent>
#include <QTextEdit>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>
#include <QEvent>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>

#include <TopoDS_Compound.hxx>
#include <TopoDS_Shape.hxx>

#include <map>
#include <utility>

namespace
{
struct BatchAssemblyCheckSettings
{
    QString folderPath;
    QString orderFilePath;
    QString infoFilePath = QStringLiteral("d:/info_si_assembly.txt");
};

QString inputTypeFromFilePath(const QString& filePath)
{
    const QString ext = QFileInfo(filePath).suffix().toLower();
    if (ext == QStringLiteral("stp") || ext == QStringLiteral("step"))
    {
        return QStringLiteral("step");
    }
    return QStringLiteral("brep");
}

bool isStepFilePath(const QString& filePath)
{
    const QString ext = QFileInfo(filePath).suffix().toLower();
    return ext == QStringLiteral("stp") || ext == QStringLiteral("step");
}

bool isSupportedModelFilePath(const QString& filePath)
{
    const QString ext = QFileInfo(filePath).suffix().toLower();
    return ext == QStringLiteral("brep") || ext == QStringLiteral("stp") || ext == QStringLiteral("step");
}

QString stepStructureMessage(const BRepLoadResult& res)
{
    if (!res.isStep)
    {
        return {};
    }
    if (!res.stepStructureRead)
    {
        return QObject::tr("STEP assembly structure could not be inspected.");
    }
    if (res.hasAssembly)
    {
        return QObject::tr("STEP contains assembly (%1 assembly node(s), %2 component instance(s), %3 free root shape(s)).")
            .arg(res.assemblyCount)
            .arg(res.componentCount)
            .arg(res.freeShapeCount);
    }
    return QObject::tr("STEP does not contain assembly (%1 free root shape(s)).").arg(res.freeShapeCount);
}

QString resolvePathRelativeToDocument(const QString& rawPath, const QString& documentPath)
{
    const QFileInfo rawInfo(rawPath);
    if (rawInfo.isAbsolute())
    {
        return rawInfo.absoluteFilePath();
    }

    const QFileInfo documentInfo(documentPath);
    return QFileInfo(documentInfo.absoluteDir().absoluteFilePath(rawPath)).absoluteFilePath();
}

QString problemCategoryLabel(ProblemCategory category)
{
    switch (category)
    {
    case ProblemCategory::Boolean:
        return QObject::tr("Boolean");
    case ProblemCategory::Projection:
        return QObject::tr("Projection");
    case ProblemCategory::Classification:
        return QObject::tr("Classification");
    case ProblemCategory::Topology:
        return QObject::tr("Topology");
    case ProblemCategory::Tolerance:
        return QObject::tr("Tolerance");
    case ProblemCategory::Meshing:
        return QObject::tr("Meshing");
    case ProblemCategory::HLR:
        return QObject::tr("HLR");
    case ProblemCategory::Performance:
        return QObject::tr("Performance");
    case ProblemCategory::Crash:
        return QObject::tr("Crash");
    case ProblemCategory::Unknown:
    default:
        return QObject::tr("Unknown");
    }
}

QString conciseProblemText(const std::string& text, int maxChars)
{
    QString s = QString::fromStdString(text).simplified();
    if (s.size() <= maxChars)
    {
        return s;
    }
    return s.left(maxChars - 3) + QStringLiteral("...");
}

QStringList collectBatchStepFiles(const QString& folderPath, const QString& orderFilePath, QString* errorOut)
{
    QStringList files;
    const QDir folder(folderPath);
    if (!folder.exists())
    {
        if (errorOut != nullptr)
        {
            *errorOut = QObject::tr("Folder does not exist: %1").arg(folderPath);
        }
        return files;
    }

    if (orderFilePath.trimmed().isEmpty())
    {
        const QFileInfoList entries = folder.entryInfoList(
            {QStringLiteral("*.stp"), QStringLiteral("*.step")},
            QDir::Files,
            QDir::Name | QDir::IgnoreCase);
        for (const QFileInfo& entry : entries)
        {
            files.push_back(entry.absoluteFilePath());
        }

        const QFileInfoList subdirs =
            folder.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
        for (const QFileInfo& subdir : subdirs)
        {
            QString subdirError;
            files.append(collectBatchStepFiles(subdir.absoluteFilePath(), QString(), &subdirError));
            if (!subdirError.isEmpty() && errorOut != nullptr)
            {
                *errorOut = subdirError;
                return {};
            }
        }
        return files;
    }

    QFile orderFile(orderFilePath);
    if (!orderFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if (errorOut != nullptr)
        {
            *errorOut = QObject::tr("Could not open order file: %1").arg(orderFile.errorString());
        }
        return {};
    }

    QTextStream in(&orderFile);
    while (!in.atEnd())
    {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty())
        {
            continue;
        }

        const QFileInfo item(line);
        if (item.isAbsolute())
        {
            files.push_back(item.absoluteFilePath());
            continue;
        }

        const QString directPath = folder.absoluteFilePath(line);
        if (QFileInfo::exists(directPath))
        {
            files.push_back(directPath);
            continue;
        }

        const QFileInfoList matches = folder.entryInfoList(
            {line},
            QDir::Files,
            QDir::Name | QDir::IgnoreCase);
        if (!matches.isEmpty())
        {
            files.push_back(matches.first().absoluteFilePath());
            continue;
        }

        QDirIterator it(folder.absolutePath(), {line}, QDir::Files, QDirIterator::Subdirectories);
        files.push_back(it.hasNext() ? it.next() : directPath);
    }
    return files;
}

QString batchAssemblyStatusText(const StepAssemblyInspectResult& res)
{
    if (!res.ok)
    {
        return QStringLiteral("ERROR");
    }
    if (!res.isStep)
    {
        return QStringLiteral("NOT_STEP");
    }
    if (!res.stepStructureRead)
    {
        return QStringLiteral("UNKNOWN");
    }
    return res.hasAssembly ? QStringLiteral("ASSEMBLY") : QStringLiteral("PART");
}

QString paddedReportCell(QString text, int width)
{
    text.replace('\t', ' ');
    text.replace('\r', ' ');
    text.replace('\n', ' ');
    if (text.size() >= width)
    {
        return text + QStringLiteral("  ");
    }
    return text.leftJustified(width, QLatin1Char(' '));
}

void writeBatchAssemblyReportRow(
    QTextStream& out,
    const QString& fileName,
    const QString& status,
    const QString& hasAssembly,
    const QString& assemblyCount,
    const QString& componentCount,
    const QString& freeRootShapeCount,
    const QString& message,
    const QString& filePath)
{
    out << paddedReportCell(fileName, 36) << paddedReportCell(status, 14)
        << paddedReportCell(hasAssembly, 14) << paddedReportCell(assemblyCount, 16)
        << paddedReportCell(componentCount, 16) << paddedReportCell(freeRootShapeCount, 22)
        << paddedReportCell(message, 44) << filePath << '\n';
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

void fillMissingBuildMetadata(ProblemContext& ctx)
{
    ProblemContext defaults;
    fillBuildMetadata(defaults);
    if (ctx.occtVersion.empty())
    {
        ctx.occtVersion = defaults.occtVersion;
    }
    if (ctx.compiler.empty())
    {
        ctx.compiler = defaults.compiler;
    }
    if (ctx.buildType.empty())
    {
        ctx.buildType = defaults.buildType;
    }
}

void populateProblemCategoryCombo(QComboBox* combo, ProblemCategory selected)
{
    struct Item
    {
        const char* label;
        ProblemCategory category;
    };
    const Item items[] = {
        {"Unknown", ProblemCategory::Unknown},
        {"Boolean", ProblemCategory::Boolean},
        {"Projection", ProblemCategory::Projection},
        {"Classification", ProblemCategory::Classification},
        {"Topology", ProblemCategory::Topology},
        {"Tolerance", ProblemCategory::Tolerance},
        {"Meshing", ProblemCategory::Meshing},
        {"HLR", ProblemCategory::HLR},
        {"Performance", ProblemCategory::Performance},
        {"Crash", ProblemCategory::Crash},
    };

    int selectedIndex = 0;
    for (const Item& item : items)
    {
        combo->addItem(QString::fromLatin1(item.label), static_cast<int>(item.category));
        if (item.category == selected)
        {
            selectedIndex = combo->count() - 1;
        }
    }
    combo->setCurrentIndex(selectedIndex);
}

ProblemCategory selectedProblemCategory(const QComboBox* combo)
{
    return static_cast<ProblemCategory>(combo->currentData().toInt());
}

QString inputFilesText(const std::vector<std::string>& inputFiles)
{
    QStringList lines;
    for (const std::string& input : inputFiles)
    {
        if (!input.empty())
        {
            lines.push_back(QString::fromStdString(input));
        }
    }
    return lines.join(QLatin1Char('\n'));
}

std::vector<std::string> inputFilesFromText(const QString& text)
{
    std::vector<std::string> result;
    const QStringList lines = text.split(QLatin1Char('\n'));
    result.reserve(lines.size());
    for (const QString& line : lines)
    {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty())
        {
            result.push_back(trimmed.toStdString());
        }
    }
    return result;
}

bool isReservedProblemParameter(const QString& key)
{
    const QString normalized = key.trimmed().toLower();
    return normalized == QStringLiteral("reproductionsteps") || normalized == QStringLiteral("notes")
        || normalized == QStringLiteral("environment");
}

void appendParameterRow(QTableWidget* table, const QString& key = QString(), const QString& value = QString())
{
    const int row = table->rowCount();
    table->insertRow(row);
    table->setItem(row, 0, new QTableWidgetItem(key));
    table->setItem(row, 1, new QTableWidgetItem(value));
}

std::map<std::string, std::string> parametersFromTable(const QTableWidget* table)
{
    std::map<std::string, std::string> result;
    for (int row = 0; row < table->rowCount(); ++row)
    {
        const QTableWidgetItem* keyItem = table->item(row, 0);
        const QTableWidgetItem* valueItem = table->item(row, 1);
        const QString key = keyItem == nullptr ? QString() : keyItem->text().trimmed();
        const QString value = valueItem == nullptr ? QString() : valueItem->text().trimmed();
        if (!key.isEmpty() && !isReservedProblemParameter(key))
        {
            result[key.toStdString()] = value.toStdString();
        }
    }
    return result;
}
} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setMinimumSize(1024, 700);
    applyProblemDefaults();

    auto* central = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    m_problemBanner = new QLabel(central);
    m_problemBanner->setObjectName(QStringLiteral("ProblemBanner"));
    m_problemBanner->setWordWrap(true);
    m_problemBanner->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_problemBanner->setStyleSheet(
        QStringLiteral("QLabel#ProblemBanner { border: 1px solid #b8c2cc; border-radius: 4px; "
                       "padding: 6px 8px; background: #f5f7fa; color: #17202a; }"));
    mainLayout->addWidget(m_problemBanner);

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

    mainLayout->addWidget(horiz, 1);
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

    auto* createProblemAct = fileMenu->addAction(tr("Create problem document..."));
    createProblemAct->setShortcut(tr("Ctrl+Shift+N"));
    connect(createProblemAct, &QAction::triggered, this, &MainWindow::onCreateProblemDocument);

    auto* importProblemAct = fileMenu->addAction(tr("Import problem document..."));
    importProblemAct->setShortcut(tr("Ctrl+I"));
    connect(importProblemAct, &QAction::triggered, this, &MainWindow::onImportProblemDocument);

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
    connect(diagMenu->addAction(tr("Batch check STEP &assemblies...")),
            &QAction::triggered,
            this,
            &MainWindow::onBatchCheckStepAssemblies);

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

    updateProblemBanner();
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
    m_problem.expectedBehavior.clear();
    m_problem.actualBehavior.clear();
    m_problem.inputFiles.clear();
    m_problem.parameters.clear();
    fillBuildMetadata(m_problem);
}

void MainWindow::updateWindowTitle()
{
    const QString title = QString::fromStdString(m_problem.title).trimmed();
    if (title.isEmpty())
    {
        setWindowTitle(tr("OCCTDebug - Problem analysis"));
        return;
    }
    setWindowTitle(tr("OCCTDebug - %1").arg(title));
}

void MainWindow::updateProblemBanner()
{
    updateWindowTitle();
    if (m_problemBanner == nullptr)
    {
        return;
    }

    const QString title = QString::fromStdString(m_problem.title).trimmed();
    QString text = title.isEmpty()
        ? tr("Current problem: not defined")
        : tr("Current problem: %1").arg(title);
    text += tr(" | Category: %1").arg(problemCategoryLabel(m_problem.category));
    text += tr(" | Input files: %1").arg(m_problem.inputFiles.size());

    const QString summary = conciseProblemText(m_problem.description, 180);
    if (!summary.isEmpty())
    {
        text += tr("\nSymptom: %1").arg(summary);
    }

    m_problemBanner->setText(text);
}

bool MainWindow::openBrepPath(const QString& path, QString* errorOut)
{
    return openModelPaths(QStringList{path}, errorOut);
}

bool MainWindow::openModelPaths(const QStringList& paths, QString* errorOut)
{
    QStringList normalizedPaths;
    for (const QString& path : paths)
    {
        const QString trimmed = path.trimmed();
        if (trimmed.isEmpty())
        {
            continue;
        }
        normalizedPaths.push_back(QFileInfo(trimmed).absoluteFilePath());
    }

    if (normalizedPaths.isEmpty())
    {
        if (errorOut != nullptr)
        {
            *errorOut = tr("No model file was provided.");
        }
        return false;
    }

    std::vector<TopoDS_Shape> shapes;
    QStringList loadedPaths;
    QStringList warnings;
    QStringList structureMessages;
    shapes.reserve(static_cast<size_t>(normalizedPaths.size()));

    m_lastImportStructureMessage.clear();
    for (const QString& path : normalizedPaths)
    {
        if (!QFileInfo::exists(path))
        {
            warnings.push_back(tr("Model file does not exist: %1").arg(path));
            continue;
        }
        if (!isSupportedModelFilePath(path))
        {
            warnings.push_back(tr("Unsupported model file type: %1").arg(path));
            continue;
        }

        const BRepLoadResult res = BRepLoader::loadFile(path);
        if (!res.ok)
        {
            warnings.push_back(tr("Could not load %1: %2").arg(path, res.errorMessage));
            continue;
        }

        shapes.push_back(res.shape);
        loadedPaths.push_back(path);
        const QString stepMessage = stepStructureMessage(res);
        if (!stepMessage.isEmpty())
        {
            structureMessages.push_back(stepMessage);
            Logger::info(tr("%1: %2").arg(QFileInfo(path).fileName(), stepMessage));
        }
    }

    for (const QString& warning : warnings)
    {
        Logger::warning(warning);
    }

    if (shapes.empty())
    {
        if (errorOut != nullptr)
        {
            *errorOut = warnings.isEmpty() ? tr("No model file could be loaded.") : warnings.join(QLatin1Char('\n'));
        }
        return false;
    }

    TopoDS_Shape rootShape;
    if (shapes.size() == 1)
    {
        rootShape = shapes.front();
    }
    else
    {
        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);
        for (const TopoDS_Shape& shape : shapes)
        {
            builder.Add(compound, shape);
        }
        rootShape = compound;
    }

    ShapeInspector::BuildFromShape(m_document, rootShape);
    m_shapeTree->rebuildFromDocument(m_document);
    m_propertyPanel->showShape(m_document, -1);
    m_diagnosticPanel->setFindings({});
    m_findings.clear();

    m_problem.inputFiles.clear();
    for (const QString& loadedPath : loadedPaths)
    {
        m_problem.inputFiles.push_back(loadedPath.toStdString());
    }

    if (loadedPaths.size() > 1)
    {
        m_lastImportStructureMessage = tr("Loaded %1 input model(s) into one compound for this problem.")
                                           .arg(loadedPaths.size());
    }
    else if (!structureMessages.isEmpty())
    {
        m_lastImportStructureMessage = structureMessages.first();
    }

    m_viewer->setRootShape(rootShape);
    m_viewer->setHighlightShape(TopoDS_Shape());
    m_selectedShapeId = -1;
    if (m_topologyPanel != nullptr)
    {
        m_topologyPanel->inspect(m_document, -1);
    }
    m_viewer->refreshPresentation();
    updateProblemBanner();
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
    if (!m_lastImportStructureMessage.isEmpty())
    {
        Logger::info(m_lastImportStructureMessage);
    }
    statusBar()->showMessage(
        m_lastImportStructureMessage.isEmpty()
            ? tr("Loaded %1").arg(path)
            : tr("Loaded %1 — %2").arg(path, m_lastImportStructureMessage));
}

void MainWindow::onCreateProblemDocument()
{
    ProblemContext initial = m_problem;
    fillMissingBuildMetadata(initial);

    const auto parameterText = [&initial](const std::string& key) -> QString {
        const auto it = initial.parameters.find(key);
        return it == initial.parameters.end() ? QString() : QString::fromStdString(it->second);
    };

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Create problem document"));
    dialog.resize(760, 720);

    auto* layout = new QVBoxLayout(&dialog);
    auto* form = new QFormLayout();
    layout->addLayout(form);

    auto* titleEdit = new QLineEdit(QString::fromStdString(initial.title), &dialog);
    form->addRow(tr("Title"), titleEdit);

    auto* categoryCombo = new QComboBox(&dialog);
    populateProblemCategoryCombo(categoryCombo, initial.category);
    form->addRow(tr("Category"), categoryCombo);

    auto* occtVersionEdit = new QLineEdit(QString::fromStdString(initial.occtVersion), &dialog);
    form->addRow(tr("OCCT version"), occtVersionEdit);

    auto* compilerEdit = new QLineEdit(QString::fromStdString(initial.compiler), &dialog);
    form->addRow(tr("Compiler"), compilerEdit);

    auto* buildTypeEdit = new QLineEdit(QString::fromStdString(initial.buildType), &dialog);
    form->addRow(tr("Build type"), buildTypeEdit);

    auto* inputFilesEdit = new QTextEdit(inputFilesText(initial.inputFiles), &dialog);
    inputFilesEdit->setAcceptRichText(false);
    inputFilesEdit->setMinimumHeight(64);
    form->addRow(tr("Input files"), inputFilesEdit);

    auto* summaryEdit = new QTextEdit(QString::fromStdString(initial.description), &dialog);
    summaryEdit->setAcceptRichText(false);
    summaryEdit->setMinimumHeight(84);
    form->addRow(tr("Summary"), summaryEdit);

    auto* reproEdit = new QTextEdit(parameterText("reproductionSteps"), &dialog);
    reproEdit->setAcceptRichText(false);
    reproEdit->setMinimumHeight(96);
    form->addRow(tr("Reproduction steps"), reproEdit);

    auto* expectedEdit = new QTextEdit(QString::fromStdString(initial.expectedBehavior), &dialog);
    expectedEdit->setAcceptRichText(false);
    expectedEdit->setMinimumHeight(72);
    form->addRow(tr("Expected behavior"), expectedEdit);

    auto* actualEdit = new QTextEdit(QString::fromStdString(initial.actualBehavior), &dialog);
    actualEdit->setAcceptRichText(false);
    actualEdit->setMinimumHeight(72);
    form->addRow(tr("Actual behavior"), actualEdit);

    auto* notesEdit = new QTextEdit(parameterText("notes"), &dialog);
    notesEdit->setAcceptRichText(false);
    notesEdit->setMinimumHeight(84);
    form->addRow(tr("Notes / suspected area"), notesEdit);

    auto* parameterPanel = new QWidget(&dialog);
    auto* parameterLayout = new QVBoxLayout(parameterPanel);
    parameterLayout->setContentsMargins(0, 0, 0, 0);
    auto* parameterTable = new QTableWidget(parameterPanel);
    parameterTable->setColumnCount(2);
    parameterTable->setHorizontalHeaderLabels({tr("Name"), tr("Value")});
    parameterTable->horizontalHeader()->setStretchLastSection(true);
    parameterTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    parameterTable->setMinimumHeight(110);
    for (const auto& item : initial.parameters)
    {
        const QString key = QString::fromStdString(item.first);
        if (!isReservedProblemParameter(key))
        {
            appendParameterRow(parameterTable, key, QString::fromStdString(item.second));
        }
    }
    auto* parameterButtons = new QHBoxLayout();
    auto* addParameterButton = new QPushButton(tr("Add property"), parameterPanel);
    auto* removeParameterButton = new QPushButton(tr("Remove selected"), parameterPanel);
    parameterButtons->addWidget(addParameterButton);
    parameterButtons->addWidget(removeParameterButton);
    parameterButtons->addStretch(1);
    parameterLayout->addWidget(parameterTable);
    parameterLayout->addLayout(parameterButtons);
    form->addRow(tr("Custom properties"), parameterPanel);
    connect(addParameterButton, &QPushButton::clicked, parameterTable, [parameterTable]() {
        appendParameterRow(parameterTable);
        parameterTable->setCurrentCell(parameterTable->rowCount() - 1, 0);
        parameterTable->editItem(parameterTable->item(parameterTable->rowCount() - 1, 0));
    });
    connect(removeParameterButton, &QPushButton::clicked, parameterTable, [parameterTable]() {
        const QModelIndexList selectedRows = parameterTable->selectionModel()->selectedRows();
        for (auto it = selectedRows.rbegin(); it != selectedRows.rend(); ++it)
        {
            parameterTable->removeRow(it->row());
        }
    });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    ProblemDocument document;
    document.context.title = titleEdit->text().trimmed().toStdString();
    if (document.context.title.empty())
    {
        document.context.title = "Untitled OCCT problem";
    }
    document.context.category = selectedProblemCategory(categoryCombo);
    document.context.occtVersion = occtVersionEdit->text().trimmed().toStdString();
    document.context.compiler = compilerEdit->text().trimmed().toStdString();
    document.context.buildType = buildTypeEdit->text().trimmed().toStdString();
    document.context.inputFiles = inputFilesFromText(inputFilesEdit->toPlainText());
    document.context.description = summaryEdit->toPlainText().trimmed().toStdString();
    document.context.expectedBehavior = expectedEdit->toPlainText().trimmed().toStdString();
    document.context.actualBehavior = actualEdit->toPlainText().trimmed().toStdString();
    document.context.parameters = parametersFromTable(parameterTable);
    document.reproductionSteps = reproEdit->toPlainText().trimmed().toStdString();
    document.notes = notesEdit->toPlainText().trimmed().toStdString();
    if (!document.reproductionSteps.empty())
    {
        document.context.parameters["reproductionSteps"] = document.reproductionSteps;
    }
    if (!document.notes.empty())
    {
        document.context.parameters["notes"] = document.notes;
    }

    QString defaultPath = QStringLiteral("problem.md");
    if (!document.context.inputFiles.empty())
    {
        defaultPath = QFileInfo(QString::fromStdString(document.context.inputFiles.front()))
                          .absoluteDir()
                          .absoluteFilePath(QStringLiteral("problem.md"));
    }

    const QString path = QFileDialog::getSaveFileName(
        this,
        tr("Save problem document"),
        defaultPath,
        tr("Markdown (*.md);;All files (*)"));
    if (path.isEmpty())
    {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        QMessageBox::warning(this, tr("Problem document"), file.errorString());
        return;
    }

    const QByteArray markdownBytes = QString::fromStdString(ProblemDocumentImporter::toMarkdown(document)).toUtf8();
    if (file.write(markdownBytes) != markdownBytes.size())
    {
        QMessageBox::warning(this, tr("Problem document"), file.errorString());
        return;
    }

    m_problem = document.context;
    updateProblemBanner();
    Logger::info(tr("Saved problem document: %1").arg(path));
    statusBar()->showMessage(tr("Saved problem document %1").arg(path));
}

void MainWindow::onImportProblemDocument()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        tr("Import problem document"),
        QString(),
        tr("Problem documents (*.md *.markdown *.txt);;All files (*)"));
    if (path.isEmpty())
    {
        return;
    }

    ProblemDocument document;
    std::string error;
    if (!ProblemDocumentImporter::loadFile(path.toStdString(), &document, &error))
    {
        QMessageBox::warning(this, tr("Problem document"), QString::fromStdString(error));
        return;
    }

    const std::vector<std::string> previousInputFiles = m_problem.inputFiles;
    ProblemContext importedProblem = document.context;
    std::vector<std::string> resolvedInputs;
    resolvedInputs.reserve(importedProblem.inputFiles.size());
    for (const std::string& input : importedProblem.inputFiles)
    {
        const QString rawPath = QString::fromStdString(input).trimmed();
        if (rawPath.isEmpty())
        {
            continue;
        }
        resolvedInputs.push_back(resolvePathRelativeToDocument(rawPath, path).toStdString());
    }
    importedProblem.inputFiles = resolvedInputs.empty() ? previousInputFiles : resolvedInputs;
    fillMissingBuildMetadata(importedProblem);

    m_problem = std::move(importedProblem);
    m_findings.clear();
    m_diagnosticPanel->setFindings({});
    m_sessionFilePath.clear();

    for (const std::string& warning : document.warnings)
    {
        Logger::warning(QString::fromStdString(warning));
    }

    const std::vector<std::string> importedInputFiles = m_problem.inputFiles;
    QStringList loadCandidates;
    for (const std::string& input : importedInputFiles)
    {
        const QString candidate = QString::fromStdString(input);
        if (candidate.trimmed().isEmpty())
        {
            continue;
        }
        loadCandidates.push_back(candidate);
    }

    QString loadError;
    if (!loadCandidates.isEmpty() && openModelPaths(loadCandidates, &loadError))
    {
        const int loadedCount = static_cast<int>(m_problem.inputFiles.size());
        m_problem.inputFiles = importedInputFiles;
        updateProblemBanner();
        Logger::info(tr("Imported problem document: %1").arg(path));
        Logger::info(tr("Loaded %1 model input(s) from problem document.").arg(loadedCount));
        statusBar()->showMessage(tr("Imported %1 and loaded %2 model input(s)").arg(path).arg(loadedCount));
        return;
    }

    updateProblemBanner();
    Logger::info(tr("Imported problem document: %1").arg(path));
    if (!loadError.isEmpty())
    {
        statusBar()->showMessage(tr("Imported %1; model load failed: %2").arg(path, loadError));
    }
    else
    {
        statusBar()->showMessage(tr("Imported %1").arg(path));
    }
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

    QStringList modelPaths;
    const auto appendResolvedModelPath = [&modelPaths, &path](const QString& rawPath) {
        const QString resolved = SessionSerializer::resolveInputPath(rawPath, path);
        if (!modelPaths.contains(resolved))
        {
            modelPaths.push_back(resolved);
        }
    };

    for (const SessionInput& in : session.inputs)
    {
        appendResolvedModelPath(QString::fromStdString(in.path));
    }
    if (modelPaths.isEmpty())
    {
        for (const std::string& fp : session.problem.inputFiles)
        {
            appendResolvedModelPath(QString::fromStdString(fp));
        }
    }

    if (modelPaths.isEmpty())
    {
        QMessageBox::warning(
            this,
            tr("Session"),
            tr("Could not find the model file for this session. Check paths relative to the session file."));
        return;
    }

    if (!openModelPaths(modelPaths, &err))
    {
        QMessageBox::warning(this, tr("Open failed"), err);
        return;
    }
    const std::vector<std::string> loadedInputFiles = m_problem.inputFiles;

    m_problem = session.problem;
    m_problem.inputFiles = loadedInputFiles;
    m_findings = std::move(session.diagnostics);
    m_diagnosticPanel->setFindings(m_findings);
    updateProblemBanner();

    m_sessionFilePath = path;

    if (session.selectedShapeId >= 0 && m_document.FindNode(session.selectedShapeId) != nullptr)
    {
        m_shapeTree->selectShapeId(session.selectedShapeId);
    }

    Logger::info(tr("Opened session: %1").arg(path));
    if (!m_lastImportStructureMessage.isEmpty())
    {
        Logger::info(m_lastImportStructureMessage);
    }
    statusBar()->showMessage(
        m_lastImportStructureMessage.isEmpty()
            ? tr("Session %1").arg(path)
            : tr("Session %1 — %2").arg(path, m_lastImportStructureMessage));
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

void MainWindow::onBatchCheckStepAssemblies()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Batch check STEP assemblies"));

    auto* layout = new QVBoxLayout(&dialog);
    auto* form = new QFormLayout();
    layout->addLayout(form);

    auto* folderEdit = new QLineEdit(&dialog);
    auto* folderBrowse = new QPushButton(tr("Browse..."), &dialog);
    auto* folderRow = new QWidget(&dialog);
    auto* folderLayout = new QHBoxLayout(folderRow);
    folderLayout->setContentsMargins(0, 0, 0, 0);
    folderLayout->addWidget(folderEdit);
    folderLayout->addWidget(folderBrowse);
    form->addRow(tr("Folder"), folderRow);

    auto* orderEdit = new QLineEdit(&dialog);
    auto* orderBrowse = new QPushButton(tr("Browse..."), &dialog);
    auto* orderRow = new QWidget(&dialog);
    auto* orderLayout = new QHBoxLayout(orderRow);
    orderLayout->setContentsMargins(0, 0, 0, 0);
    orderLayout->addWidget(orderEdit);
    orderLayout->addWidget(orderBrowse);
    form->addRow(tr("Order file"), orderRow);

    auto* infoEdit = new QLineEdit(QStringLiteral("d:/info_si_assembly.txt"), &dialog);
    auto* infoBrowse = new QPushButton(tr("Browse..."), &dialog);
    auto* infoRow = new QWidget(&dialog);
    auto* infoLayout = new QHBoxLayout(infoRow);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->addWidget(infoEdit);
    infoLayout->addWidget(infoBrowse);
    form->addRow(tr("Info file"), infoRow);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);

    connect(folderBrowse, &QPushButton::clicked, &dialog, [&dialog, folderEdit]() {
        const QString selected =
            QFileDialog::getExistingDirectory(&dialog, QObject::tr("Select folder"), folderEdit->text());
        if (!selected.isEmpty())
        {
            folderEdit->setText(selected);
        }
    });

    connect(orderBrowse, &QPushButton::clicked, &dialog, [&dialog, orderEdit, folderEdit]() {
        const QString selected = QFileDialog::getOpenFileName(
            &dialog,
            QObject::tr("Select order file"),
            orderEdit->text().isEmpty() ? folderEdit->text() : orderEdit->text(),
            QObject::tr("Text files (*.txt);;All files (*)"));
        if (!selected.isEmpty())
        {
            orderEdit->setText(selected);
        }
    });

    connect(infoBrowse, &QPushButton::clicked, &dialog, [&dialog, infoEdit]() {
        const QString selected = QFileDialog::getSaveFileName(
            &dialog,
            QObject::tr("Select info file"),
            infoEdit->text(),
            QObject::tr("Text files (*.txt);;All files (*)"));
        if (!selected.isEmpty())
        {
            infoEdit->setText(selected);
        }
    });

    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&dialog, folderEdit, infoEdit]() {
        if (folderEdit->text().trimmed().isEmpty())
        {
            QMessageBox::warning(&dialog, QObject::tr("Batch check"), QObject::tr("Select a folder first."));
            return;
        }
        if (infoEdit->text().trimmed().isEmpty())
        {
            QMessageBox::warning(&dialog, QObject::tr("Batch check"), QObject::tr("Select an info file first."));
            return;
        }
        dialog.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    const BatchAssemblyCheckSettings settings{
        folderEdit->text().trimmed(),
        orderEdit->text().trimmed(),
        infoEdit->text().trimmed(),
    };

    QString err;
    const QStringList files = collectBatchStepFiles(settings.folderPath, settings.orderFilePath, &err);
    if (!err.isEmpty())
    {
        QMessageBox::warning(this, tr("Batch check"), err);
        return;
    }
    if (files.isEmpty())
    {
        QMessageBox::information(this, tr("Batch check"), tr("No files to check."));
        return;
    }

    QFile infoFile(settings.infoFilePath);
    if (!infoFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(
            this,
            tr("Batch check"),
            tr("Could not open info file for writing: %1").arg(infoFile.errorString()));
        return;
    }

    QTextStream out(&infoFile);
    writeBatchAssemblyReportRow(
        out,
        QStringLiteral("file_name"),
        QStringLiteral("status"),
        QStringLiteral("has_assembly"),
        QStringLiteral("assembly_count"),
        QStringLiteral("component_count"),
        QStringLiteral("free_root_shape_count"),
        QStringLiteral("message"),
        QStringLiteral("file_path"));

    QProgressDialog progress(tr("Checking STEP assemblies..."), tr("Cancel"), 0, files.size(), this);
    progress.setWindowModality(Qt::ApplicationModal);
    progress.setMinimumDuration(0);

    int checkedCount = 0;
    int assemblyCount = 0;
    int errorCount = 0;
    for (const QString& filePath : files)
    {
        progress.setValue(checkedCount);
        progress.setLabelText(tr("Checking %1").arg(QFileInfo(filePath).fileName()));
        qApp->processEvents();
        if (progress.wasCanceled())
        {
            break;
        }

        QFileInfo fileInfo(filePath);
        QString message;
        StepAssemblyInspectResult res;
        if (!fileInfo.exists())
        {
            message = tr("File does not exist.");
            ++errorCount;
            writeBatchAssemblyReportRow(
                out,
                fileInfo.fileName(),
                QStringLiteral("MISSING"),
                QString(),
                QString(),
                QString(),
                QString(),
                message,
                filePath);
            ++checkedCount;
            continue;
        }
        if (!isStepFilePath(filePath))
        {
            message = tr("Not a STEP/STP file.");
            writeBatchAssemblyReportRow(
                out,
                fileInfo.fileName(),
                QStringLiteral("NOT_STEP"),
                QStringLiteral("false"),
                QStringLiteral("0"),
                QStringLiteral("0"),
                QStringLiteral("0"),
                message,
                fileInfo.absoluteFilePath());
            ++checkedCount;
            continue;
        }

        res = BRepLoader::inspectStepAssembly(fileInfo.absoluteFilePath());
        const QString status = batchAssemblyStatusText(res);
        if (!res.ok)
        {
            message = res.errorMessage;
            ++errorCount;
        }
        else if (!res.stepStructureRead)
        {
            message = tr("STEP assembly structure could not be inspected.");
            ++errorCount;
        }
        else if (res.hasAssembly)
        {
            message = tr("Contains assembly.");
            ++assemblyCount;
        }
        else
        {
            message = tr("Does not contain assembly.");
        }

        writeBatchAssemblyReportRow(
            out,
            fileInfo.fileName(),
            status,
            res.hasAssembly ? QStringLiteral("true") : QStringLiteral("false"),
            QString::number(res.assemblyCount),
            QString::number(res.componentCount),
            QString::number(res.freeShapeCount),
            message,
            fileInfo.absoluteFilePath());
        ++checkedCount;
    }

    progress.setValue(files.size());
    infoFile.close();

    const QString summary = tr("Checked %1 file(s), %2 assembly file(s), %3 error(s). Info written to %4")
                                .arg(checkedCount)
                                .arg(assemblyCount)
                                .arg(errorCount)
                                .arg(settings.infoFilePath);
    Logger::info(summary);
    statusBar()->showMessage(summary);
    QMessageBox::information(this, tr("Batch check"), summary);
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
