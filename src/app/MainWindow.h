#pragma once

#include "core/DiagnosticFinding.h"
#include "core/ProblemContext.h"
#include "core/ShapeDocument.h"
#include "diagnose/DiagnosticEngine.h"

#include <QMainWindow>

#include <QString>

class QEvent;
class QShowEvent;

#include <vector>
class DiagnosticPanel;
class PropertyPanel;
class QDockWidget;
class ShapeTreeWidget;
class ViewerWidget;
class TopologyDetailPanel;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;
    void changeEvent(QEvent* event) override;

private slots:
    void onOpenBrep();
    void onRunDiagnostics();
    void onExportMarkdown();
    void onExportShapeJson();
    void onExportMinimalRepro();
    void onSaveSession();
    void onOpenSession();
    void onShapeSelected(int shapeId);
    void onFindingActivated(int shapeId);

private:
    void applyProblemDefaults();
    void updateWindowTitle();
    bool openBrepPath(const QString& path, QString* errorOut);

    ShapeDocument m_document;
    ProblemContext m_problem;
    std::vector<DiagnosticFinding> m_findings;
    DiagnosticEngine m_engine;

    ShapeTreeWidget* m_shapeTree = nullptr;
    PropertyPanel* m_propertyPanel = nullptr;
    DiagnosticPanel* m_diagnosticPanel = nullptr;
    ViewerWidget* m_viewer = nullptr;
    QDockWidget* m_topologyDock = nullptr;
    TopologyDetailPanel* m_topologyPanel = nullptr;

    QString m_sessionFilePath;
    int m_selectedShapeId = -1;

    /// For focusObjectChanged (Qt6): previous focused widget to detect leaving/entering the viewer.
    QWidget* m_prevFocusWidget = nullptr;
};
