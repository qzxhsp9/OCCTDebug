#pragma once

#include "core/runner/CommandRunner.h"

#include <QString>

namespace occtdebug
{
struct WorkbenchMockData;

struct TaskHistoryStartInput
{
    QString id;
    QString title;
    CommandRequest request;
    QString artifact;
    QString stdoutLog;
    QString stderrLog;
    int maxRecords = 200;
};

struct TaskHistoryFinishInput
{
    QString id;
    CommandResult result;
    QString artifact;
    QString stdoutLog;
    QString stderrLog;
    QString note;
    int maxRecords = 200;
};

class TaskHistoryCoordinator
{
public:
    static void recordStarted(WorkbenchMockData& data, const TaskHistoryStartInput& input);
    static void recordFinished(WorkbenchMockData& data, const TaskHistoryFinishInput& input);
    static QString outcomeStatus(const CommandResult& result);
};
} // namespace occtdebug
