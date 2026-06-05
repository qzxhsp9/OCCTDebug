#pragma once

#include "core/case/CaseManifest.h"

#include <QWidget>

class QTableWidget;

namespace occtdebug
{
class TaskHistoryPanel final : public QWidget
{
public:
    explicit TaskHistoryPanel(QWidget* parent = nullptr);

    void setTasks(const QVector<TaskRecord>& tasks);
    int taskCount() const;
    QString statusAt(int row) const;
    QString artifactAt(int row) const;

private:
    QTableWidget* m_table = nullptr;
};
} // namespace occtdebug
