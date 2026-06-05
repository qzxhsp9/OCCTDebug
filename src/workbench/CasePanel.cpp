#include "workbench/CasePanel.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QStringList>
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

void setMargins(QLayout* layout, int margin = 10, int spacing = 8)
{
    layout->setContentsMargins(margin, margin, margin, margin);
    layout->setSpacing(spacing);
}

void clearLayout(QLayout* layout)
{
    if (layout == nullptr)
    {
        return;
    }

    while (QLayoutItem* child = layout->takeAt(0))
    {
        if (QWidget* widget = child->widget())
        {
            widget->deleteLater();
        }
        delete child;
    }
}
} // namespace

namespace occtdebug
{
CasePanel::CasePanel(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("Column"));
    auto* layout = new QVBoxLayout(this);
    setMargins(layout, 10, 8);
    layout->addWidget(createCaseListPanel(), 3);
    layout->addWidget(createWorkflowPanel(), 3);
    layout->addWidget(createKeyInputPanel(), 2);
}

void CasePanel::setData(const WorkbenchMockData& data)
{
    const QVector<WorkflowStep> steps = data.manifest.workflowState.steps.isEmpty()
        ? data.workflowSteps
        : data.manifest.workflowState.steps;
    refreshWorkflow(steps);
    refreshKeyInputs(data.keyInputs);
}

void CasePanel::setCaseSummaries(const QVector<CaseSummary>& summaries, const QString& activeCaseId)
{
    if (m_caseList == nullptr)
    {
        return;
    }

    m_caseList->clear();
    for (const CaseSummary& caseSummary : summaries)
    {
        auto* entry = new QListWidgetItem(QStringLiteral("%1    %2\n%3\n创建：%4")
                .arg(caseSummary.id, caseSummary.status, caseSummary.title, caseSummary.createdAt));
        entry->setData(Qt::UserRole, caseSummary.id);
        m_caseList->addItem(entry);
        if (caseSummary.id == activeCaseId)
        {
            m_caseList->setCurrentItem(entry);
        }
    }

    if (m_caseList->currentRow() < 0 && m_caseList->count() > 0)
    {
        m_caseList->setCurrentRow(0);
    }
}

QString CasePanel::currentCaseId() const
{
    if (m_caseList == nullptr || m_caseList->currentItem() == nullptr)
    {
        return QString();
    }
    return m_caseList->currentItem()->data(Qt::UserRole).toString();
}

QWidget* CasePanel::createPanel(const QString& title, QWidget* body)
{
    auto* frame = new QFrame;
    frame->setObjectName(QStringLiteral("Panel"));
    auto* layout = new QVBoxLayout(frame);
    setMargins(layout, 10, 8);
    layout->addWidget(label(title, QStringLiteral("PanelTitle")));
    layout->addWidget(body, 1);
    return frame;
}

QWidget* CasePanel::createCaseListPanel()
{
    auto* body = new QWidget;
    auto* layout = new QVBoxLayout(body);
    setMargins(layout, 0, 6);
    layout->addWidget(label(s("搜索案例 ID / 关键词"), QStringLiteral("SearchBox")));

    auto* actions = new QWidget;
    auto* actionLayout = new QHBoxLayout(actions);
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(6);

    auto* newButton = new QPushButton(s("新建"));
    auto* openButton = new QPushButton(s("打开"));
    auto* saveButton = new QPushButton(s("保存"));
    auto* refreshButton = new QPushButton(s("刷新"));
    connect(newButton, &QPushButton::clicked, this, &CasePanel::newCaseRequested);
    connect(openButton, &QPushButton::clicked, this, &CasePanel::openCaseRequested);
    connect(saveButton, &QPushButton::clicked, this, &CasePanel::saveCaseRequested);
    connect(refreshButton, &QPushButton::clicked, this, &CasePanel::refreshRequested);
    actionLayout->addWidget(newButton);
    actionLayout->addWidget(openButton);
    actionLayout->addWidget(saveButton);
    actionLayout->addWidget(refreshButton);
    layout->addWidget(actions);

    m_caseList = new QListWidget;
    m_caseList->setObjectName(QStringLiteral("CaseList"));
    connect(m_caseList, &QListWidget::itemActivated, this, [this](QListWidgetItem* item) {
        if (item != nullptr)
        {
            emit caseActivated(item->data(Qt::UserRole).toString());
        }
    });
    connect(m_caseList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        if (item != nullptr)
        {
            emit caseActivated(item->data(Qt::UserRole).toString());
        }
    });
    layout->addWidget(m_caseList, 1);
    return createPanel(s("案例列表"), body);
}

QWidget* CasePanel::createWorkflowPanel()
{
    m_workflowTree = new QTreeWidget;
    m_workflowTree->setHeaderHidden(true);
    m_workflowTree->setIndentation(12);
    return createPanel(s("流程状态"), m_workflowTree);
}

QWidget* CasePanel::createKeyInputPanel()
{
    auto* body = new QWidget;
    m_keyInputGrid = new QGridLayout(body);
    setMargins(m_keyInputGrid, 0, 6);
    return createPanel(s("关键输入"), body);
}

void CasePanel::refreshWorkflow(const QVector<WorkflowStep>& steps)
{
    if (m_workflowTree == nullptr)
    {
        return;
    }

    m_workflowTree->clear();
    for (const WorkflowStep& step : steps)
    {
        m_workflowTree->addTopLevelItem(
            new QTreeWidgetItem(QStringList{QStringLiteral("%1 %2        %3").arg(step.marker, step.title, step.state)}));
    }
}

void CasePanel::refreshKeyInputs(const QVector<LabelValue>& inputs)
{
    clearLayout(m_keyInputGrid);
    if (m_keyInputGrid == nullptr)
    {
        return;
    }

    for (int row = 0; row < inputs.size(); ++row)
    {
        const LabelValue& input = inputs[row];
        m_keyInputGrid->addWidget(label(input.label, QStringLiteral("MutedText")), row, 0);
        m_keyInputGrid->addWidget(label(input.value), row, 1);
    }
}
} // namespace occtdebug
