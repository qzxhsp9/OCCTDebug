#pragma once

#include "core/runner/CommandRunner.h"

#include <QObject>
#include <QString>
#include <QVector>

namespace occtdebug
{
struct CommandTask
{
    QString id;
    QString title;
    QString phase;
    QString subphase;
    bool dryRun = false;
    CommandRequest request;
};

struct CommandTaskResult
{
    CommandTask task;
    CommandResult result;
};

class CommandTaskQueue final : public QObject
{
    Q_OBJECT

public:
    explicit CommandTaskQueue(QObject* parent = nullptr);

    bool isRunning() const;
    int queuedCount() const;
    CommandTask activeTask() const;
    QVector<CommandTaskResult> results() const;

    bool start(const QVector<CommandTask>& tasks, QString* error = nullptr);
    void cancelCurrent();
    void cancelAll();

signals:
    void taskStarted(const occtdebug::CommandTask& task);
    void taskFinished(const occtdebug::CommandTaskResult& result);
    void finished(const QVector<occtdebug::CommandTaskResult>& results);

private:
    void startNext();
    void handleRunnerFinished(const CommandResult& result);
    void appendSkippedQueuedTasks();
    void finishQueue();
    static CommandResult skippedResult(const CommandTask& task);

    CommandRunner m_runner;
    QVector<CommandTask> m_queue;
    QVector<CommandTaskResult> m_results;
    CommandTask m_activeTask;
    bool m_running = false;
    bool m_hasActiveTask = false;
    bool m_cancelAllRequested = false;
};
} // namespace occtdebug
