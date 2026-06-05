#include "core/runner/CommandTaskQueue.h"

namespace occtdebug
{
CommandTaskQueue::CommandTaskQueue(QObject* parent)
    : QObject(parent)
    , m_runner(this)
{
    connect(&m_runner, &CommandRunner::finished, this, &CommandTaskQueue::handleRunnerFinished);
}

bool CommandTaskQueue::isRunning() const
{
    return m_running;
}

int CommandTaskQueue::queuedCount() const
{
    return m_queue.size();
}

CommandTask CommandTaskQueue::activeTask() const
{
    return m_hasActiveTask ? m_activeTask : CommandTask();
}

QVector<CommandTaskResult> CommandTaskQueue::results() const
{
    return m_results;
}

bool CommandTaskQueue::start(const QVector<CommandTask>& tasks, QString* error)
{
    if (m_running || m_runner.isRunning())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("a task queue is already running");
        }
        return false;
    }
    if (tasks.isEmpty())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("task queue is empty");
        }
        return false;
    }

    m_queue = tasks;
    m_results.clear();
    m_activeTask = {};
    m_running = true;
    m_hasActiveTask = false;
    m_cancelAllRequested = false;
    startNext();
    return true;
}

void CommandTaskQueue::cancelCurrent()
{
    if (m_runner.isRunning())
    {
        m_runner.cancel();
    }
}

void CommandTaskQueue::cancelAll()
{
    if (!m_running)
    {
        return;
    }

    m_cancelAllRequested = true;
    if (m_runner.isRunning())
    {
        m_runner.cancel();
        return;
    }

    appendSkippedQueuedTasks();
    finishQueue();
}

void CommandTaskQueue::startNext()
{
    if (m_cancelAllRequested)
    {
        appendSkippedQueuedTasks();
        finishQueue();
        return;
    }
    if (m_queue.isEmpty())
    {
        finishQueue();
        return;
    }

    m_activeTask = m_queue.takeFirst();
    m_hasActiveTask = true;
    emit taskStarted(m_activeTask);

    QString error;
    if (m_runner.start(m_activeTask.request, &error))
    {
        return;
    }

    CommandResult failed;
    failed.program = m_activeTask.request.program;
    failed.arguments = m_activeTask.request.arguments;
    failed.workingDirectory = m_activeTask.request.workingDirectory;
    failed.timeoutMs = m_activeTask.request.timeoutMs;
    failed.exitCode = -1;
    failed.stderrText = error;
    handleRunnerFinished(failed);
}

void CommandTaskQueue::handleRunnerFinished(const CommandResult& result)
{
    if (!m_hasActiveTask)
    {
        return;
    }

    CommandTaskResult taskResult {m_activeTask, result};
    m_results.push_back(taskResult);
    emit taskFinished(taskResult);

    m_activeTask = {};
    m_hasActiveTask = false;

    startNext();
}

void CommandTaskQueue::appendSkippedQueuedTasks()
{
    while (!m_queue.isEmpty())
    {
        const CommandTask task = m_queue.takeFirst();
        CommandTaskResult taskResult {task, skippedResult(task)};
        m_results.push_back(taskResult);
        emit taskFinished(taskResult);
    }
}

void CommandTaskQueue::finishQueue()
{
    if (!m_running)
    {
        return;
    }

    m_running = false;
    m_cancelAllRequested = false;
    m_activeTask = {};
    m_hasActiveTask = false;
    emit finished(m_results);
}

CommandResult CommandTaskQueue::skippedResult(const CommandTask& task)
{
    CommandResult result;
    result.program = task.request.program;
    result.arguments = task.request.arguments;
    result.workingDirectory = task.request.workingDirectory;
    result.exitCode = -1;
    result.canceled = true;
    result.stderrText = QStringLiteral("skipped because task queue was canceled");
    result.timeoutMs = task.request.timeoutMs;
    return result;
}
} // namespace occtdebug
