#pragma once

#include "workbench/WorkbenchMockData.h"

#include <QWidget>

class QGridLayout;
class QListWidget;
class QTreeWidget;

namespace occtdebug
{
class CasePanel final : public QWidget
{
    Q_OBJECT

public:
    explicit CasePanel(QWidget* parent = nullptr);

    void setData(const WorkbenchMockData& data);
    void setCaseSummaries(const QVector<CaseSummary>& summaries, const QString& activeCaseId);
    QString currentCaseId() const;

signals:
    void newCaseRequested();
    void openCaseRequested();
    void saveCaseRequested();
    void refreshRequested();
    void caseActivated(const QString& caseId);

private:
    QWidget* createPanel(const QString& title, QWidget* body);
    QWidget* createCaseListPanel();
    QWidget* createWorkflowPanel();
    QWidget* createKeyInputPanel();
    void refreshWorkflow(const QVector<WorkflowStep>& steps);
    void refreshKeyInputs(const QVector<LabelValue>& inputs);

    QListWidget* m_caseList = nullptr;
    QTreeWidget* m_workflowTree = nullptr;
    QGridLayout* m_keyInputGrid = nullptr;
};
} // namespace occtdebug
