#include "workbench/TaskHistoryPanel.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QTableWidget>
#include <QVBoxLayout>

namespace occtdebug
{
namespace
{
QTableWidgetItem* item(const QString& text)
{
    auto* out = new QTableWidgetItem(text);
    out->setFlags(out->flags() & ~Qt::ItemIsEditable);
    return out;
}
} // namespace

TaskHistoryPanel::TaskHistoryPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);

    m_table = new QTableWidget(0, 7, this);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("Task"),
        QStringLiteral("Status"),
        QStringLiteral("Elapsed"),
        QStringLiteral("Exit"),
        QStringLiteral("Artifact"),
        QStringLiteral("Stdout"),
        QStringLiteral("Started"),
    });
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_table);
}

void TaskHistoryPanel::setTasks(const QVector<TaskRecord>& tasks)
{
    if (m_table == nullptr)
    {
        return;
    }

    m_table->setRowCount(tasks.size());
    for (int i = 0; i < tasks.size(); ++i)
    {
        const TaskRecord& task = tasks.at(tasks.size() - 1 - i);
        m_table->setItem(i, 0, item(task.title.isEmpty() ? task.id : task.title));
        m_table->setItem(i, 1, item(task.status));
        m_table->setItem(i, 2, item(task.elapsedMs > 0 ? QStringLiteral("%1 ms").arg(task.elapsedMs) : QString()));
        m_table->setItem(i, 3, item(task.status == QStringLiteral("running") ? QString() : QString::number(task.exitCode)));
        m_table->setItem(i, 4, item(task.artifact));
        m_table->setItem(i, 5, item(task.stdoutLog));
        m_table->setItem(i, 6, item(task.startedAt));
    }
    m_table->resizeColumnsToContents();
}

int TaskHistoryPanel::taskCount() const
{
    return m_table == nullptr ? 0 : m_table->rowCount();
}

QString TaskHistoryPanel::statusAt(int row) const
{
    if (m_table == nullptr || row < 0 || row >= m_table->rowCount() || m_table->item(row, 1) == nullptr)
    {
        return QString();
    }
    return m_table->item(row, 1)->text();
}

QString TaskHistoryPanel::artifactAt(int row) const
{
    if (m_table == nullptr || row < 0 || row >= m_table->rowCount() || m_table->item(row, 4) == nullptr)
    {
        return QString();
    }
    return m_table->item(row, 4)->text();
}
} // namespace occtdebug
