#pragma once

#include "core/DiagnosticFinding.h"

#include <QWidget>

#include <vector>

class QListWidget;

class DiagnosticPanel final : public QWidget
{
    Q_OBJECT

public:
    explicit DiagnosticPanel(QWidget* parent = nullptr);

    void setFindings(const std::vector<DiagnosticFinding>& findings);

signals:
    void findingActivated(int shapeId);

private slots:
    void onItemActivated();

private:
    QListWidget* m_list = nullptr;
    std::vector<DiagnosticFinding> m_findings;
};
