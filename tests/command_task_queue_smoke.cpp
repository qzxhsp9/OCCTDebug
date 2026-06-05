#include "core/runner/CommandTaskQueue.h"

#include <QCoreApplication>
#include <QDir>
#include <QTimer>

#include <iostream>

namespace
{
occtdebug::CommandTask powershellTask(const QString& id,
                                      const QString& title,
                                      const QString& phase,
                                      const QString& subphase,
                                      bool dryRun,
                                      const QString& script)
{
    occtdebug::CommandTask task;
    task.id = id;
    task.title = title;
    task.phase = phase;
    task.subphase = subphase;
    task.dryRun = dryRun;
    task.request.program = QStringLiteral("powershell.exe");
    task.request.arguments = {
        QStringLiteral("-NoProfile"),
        QStringLiteral("-ExecutionPolicy"),
        QStringLiteral("Bypass"),
        QStringLiteral("-Command"),
        script,
    };
    task.request.workingDirectory = QDir::currentPath();
    return task;
}

bool expect(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << "\n";
    }
    return condition;
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    occtdebug::CommandTaskQueue queue;
    QVector<QString> started;
    QVector<occtdebug::CommandTaskResult> orderedResults;
    bool finished = false;

    QObject::connect(&queue, &occtdebug::CommandTaskQueue::taskStarted, &app, [&](const occtdebug::CommandTask& task) {
        started.push_back(task.id);
    });
    QObject::connect(&queue, &occtdebug::CommandTaskQueue::taskFinished, &app, [&](const occtdebug::CommandTaskResult& result) {
        orderedResults.push_back(result);
    });
    QObject::connect(&queue, &occtdebug::CommandTaskQueue::finished, &app, [&](const QVector<occtdebug::CommandTaskResult>& results) {
        finished = true;
        orderedResults = results;
        app.quit();
    });

    QString error;
    if (!queue.start({
            powershellTask(QStringLiteral("gate"),
                           QStringLiteral("DRAW gate"),
                           QStringLiteral("before"),
                           QStringLiteral("gate"),
                           false,
                           QStringLiteral("Write-Output QUEUE_ONE")),
            powershellTask(QStringLiteral("dry_run"),
                           QStringLiteral("Patch dry-run"),
                           QStringLiteral("patch"),
                           QStringLiteral("dry_run"),
                           true,
                           QStringLiteral("Write-Output QUEUE_TWO")),
        },
        &error))
    {
        std::cerr << "failed to start queue: " << error.toStdString() << "\n";
        return 1;
    }

    QTimer::singleShot(10000, &app, [&]() {
        std::cerr << "task queue smoke timed out\n";
        app.quit();
    });
    app.exec();

    if (!expect(finished, "queue did not finish")
        || !expect(started.size() == 2 && started[0] == QStringLiteral("gate") && started[1] == QStringLiteral("dry_run"),
                   "queue did not preserve start order")
        || !expect(orderedResults.size() == 2, "queue did not return two results")
        || !expect(orderedResults[0].result.stdoutText.contains(QStringLiteral("QUEUE_ONE")), "first task output missing")
        || !expect(orderedResults[1].task.dryRun, "dry-run task metadata was not preserved")
        || !expect(orderedResults[1].task.phase == QStringLiteral("patch")
                       && orderedResults[1].task.subphase == QStringLiteral("dry_run"),
                   "phase/subphase metadata was not preserved")
        || !expect(!queue.isRunning(), "queue still running after finish"))
    {
        return 2;
    }

    occtdebug::CommandTaskQueue cancelQueue;
    QVector<occtdebug::CommandTaskResult> cancelResults;
    QObject::connect(&cancelQueue,
                     &occtdebug::CommandTaskQueue::finished,
                     &app,
                     [&](const QVector<occtdebug::CommandTaskResult>& results) {
                         cancelResults = results;
                         app.quit();
                     });

    if (!cancelQueue.start({
            powershellTask(QStringLiteral("slow"),
                           QStringLiteral("Slow task"),
                           QStringLiteral("before"),
                           QStringLiteral("command"),
                           false,
                           QStringLiteral("Start-Sleep -Seconds 5; Write-Output SHOULD_NOT_COMPLETE")),
            powershellTask(QStringLiteral("queued"),
                           QStringLiteral("Queued task"),
                           QStringLiteral("after"),
                           QStringLiteral("command"),
                           false,
                           QStringLiteral("Write-Output SHOULD_NOT_RUN")),
        },
        &error))
    {
        std::cerr << "failed to start cancel queue: " << error.toStdString() << "\n";
        return 3;
    }

    QTimer::singleShot(200, &app, [&]() {
        cancelQueue.cancelAll();
    });
    QTimer::singleShot(10000, &app, [&]() {
        std::cerr << "task queue cancel timed out\n";
        app.quit();
    });
    app.exec();

    if (!expect(cancelResults.size() == 2, "cancel queue should produce active and skipped results")
        || !expect(cancelResults[0].result.canceled, "active task was not marked canceled")
        || !expect(cancelResults[1].result.canceled, "queued task was not marked canceled")
        || !expect(cancelResults[1].result.stderrText.contains(QStringLiteral("skipped")), "queued task did not record skip reason")
        || !expect(!cancelResults[0].result.stdoutText.contains(QStringLiteral("SHOULD_NOT_COMPLETE")), "canceled task completed unexpectedly")
        || !expect(!cancelResults[1].result.stdoutText.contains(QStringLiteral("SHOULD_NOT_RUN")), "queued task ran unexpectedly")
        || !expect(!cancelQueue.isRunning(), "cancel queue still running"))
    {
        return 4;
    }

    std::cout << "COMMAND_TASK_QUEUE_SMOKE_OK\n";
    return 0;
}
