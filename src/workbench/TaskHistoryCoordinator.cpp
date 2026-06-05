#include "workbench/TaskHistoryCoordinator.h"

#include "workbench/WorkbenchMockData.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>

#include <algorithm>

namespace occtdebug
{
namespace
{
QString currentUtcIsoTimestamp()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}

QString caseRelativeOrFileName(const QString& workspaceRoot, const QString& path)
{
    const QFileInfo info(path);
    if (path.trimmed().isEmpty())
    {
        return {};
    }
    if (workspaceRoot.trimmed().isEmpty())
    {
        return info.fileName();
    }

    const QDir workspace(workspaceRoot);
    const QString relative = workspace.relativeFilePath(info.absoluteFilePath()).replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (!relative.startsWith(QStringLiteral("../")) && relative != QStringLiteral("..") && !QFileInfo(relative).isAbsolute())
    {
        return relative;
    }
    return info.fileName();
}

void trimHistory(WorkbenchMockData& data, int maxRecords)
{
    if (maxRecords <= 0)
    {
        return;
    }
    if (data.taskHistory.size() > maxRecords)
    {
        data.taskHistory.erase(data.taskHistory.begin(), data.taskHistory.begin() + (data.taskHistory.size() - maxRecords));
    }
}

void syncManifest(WorkbenchMockData& data)
{
    data.manifest.taskHistory = data.taskHistory;
}
} // namespace

QString TaskHistoryCoordinator::outcomeStatus(const CommandResult& result)
{
    if (result.timedOut)
    {
        return QStringLiteral("timed_out");
    }
    if (result.canceled)
    {
        return QStringLiteral("canceled");
    }
    return result.exitCode == 0 ? QStringLiteral("passed") : QStringLiteral("failed");
}

void TaskHistoryCoordinator::recordStarted(WorkbenchMockData& data, const TaskHistoryStartInput& input)
{
    TaskRecord record;
    record.id = input.id;
    record.title = input.title;
    record.status = QStringLiteral("running");
    record.program = QFileInfo(input.request.program).fileName();
    record.arguments = input.request.arguments.join(QLatin1Char(' '));
    record.workingDirectory = caseRelativeOrFileName(data.workspaceRoot, input.request.workingDirectory);
    record.startedAt = currentUtcIsoTimestamp();
    record.artifact = input.artifact;
    record.stdoutLog = input.stdoutLog;
    record.stderrLog = input.stderrLog;

    data.taskHistory.push_back(record);
    trimHistory(data, input.maxRecords);
    syncManifest(data);
}

void TaskHistoryCoordinator::recordFinished(WorkbenchMockData& data, const TaskHistoryFinishInput& input)
{
    auto match = std::find_if(data.taskHistory.rbegin(), data.taskHistory.rend(), [&](const TaskRecord& task) {
        return task.id == input.id && task.status == QStringLiteral("running");
    });

    if (match == data.taskHistory.rend())
    {
        TaskRecord record;
        record.id = input.id;
        record.title = input.id;
        record.startedAt = currentUtcIsoTimestamp();
        data.taskHistory.push_back(record);
        trimHistory(data, input.maxRecords);
        match = data.taskHistory.rbegin();
    }

    match->status = outcomeStatus(input.result);
    match->program = QFileInfo(input.result.program).fileName();
    match->arguments = input.result.arguments.join(QLatin1Char(' '));
    match->workingDirectory = caseRelativeOrFileName(data.workspaceRoot, input.result.workingDirectory);
    match->finishedAt = currentUtcIsoTimestamp();
    match->elapsedMs = input.result.elapsedMs;
    match->exitCode = input.result.exitCode;
    if (!input.artifact.isEmpty())
    {
        match->artifact = input.artifact;
    }
    if (!input.stdoutLog.isEmpty())
    {
        match->stdoutLog = input.stdoutLog;
    }
    if (!input.stderrLog.isEmpty())
    {
        match->stderrLog = input.stderrLog;
    }
    match->note = input.note;

    syncManifest(data);
}
} // namespace occtdebug
