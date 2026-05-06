#pragma once

#include "core/DiagnosticFinding.h"
#include "core/ProblemContext.h"
#include "core/ShapeDocument.h"
#include "diagnose/DiagnosticEngine.h"

#include <QMainWindow>

#include <QString>

#include <vector>
class DiagnosticPanel;
class PropertyPanel;
class ShapeTreeWidget;
class ViewerWidget;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

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

    QString m_sessionFilePath;
    int m_selectedShapeId = -1;
};
