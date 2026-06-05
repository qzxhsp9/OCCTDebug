#include "core/runner/CommandRunner.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcessEnvironment>
#include <QTimer>

#include <iostream>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    occtdebug::CommandRunner runner;
    occtdebug::CommandResult result;
    bool finished = false;

    QObject::connect(&runner, &occtdebug::CommandRunner::finished, &app, [&](const occtdebug::CommandResult& commandResult) {
        result = commandResult;
        finished = true;
        app.quit();
    });

    occtdebug::CommandRequest request;
    request.program = QStringLiteral("powershell.exe");
    request.arguments = {
        QStringLiteral("-NoProfile"),
        QStringLiteral("-ExecutionPolicy"),
        QStringLiteral("Bypass"),
        QStringLiteral("-Command"),
        QStringLiteral("Start-Sleep -Seconds 5; Write-Output SHOULD_NOT_COMPLETE"),
    };
    request.workingDirectory = QDir::currentPath();
    request.environment = QProcessEnvironment::systemEnvironment();
    request.timeoutMs = 200;

    QString error;
    if (!runner.start(request, &error))
    {
        std::cerr << "failed to start command: " << error.toStdString() << "\n";
        return 1;
    }

    QTimer::singleShot(6000, &app, [&]() {
        std::cerr << "command timeout smoke did not finish\n";
        app.quit();
    });

    app.exec();

    if (!finished)
    {
        return 2;
    }
    if (!result.timedOut)
    {
        std::cerr << "result was not marked timed out\n";
        return 3;
    }
    if (result.canceled)
    {
        std::cerr << "timeout was incorrectly marked canceled\n";
        return 4;
    }
    if (result.timeoutMs != request.timeoutMs)
    {
        std::cerr << "timeout metadata mismatch\n";
        return 5;
    }
    if (result.stdoutText.contains(QStringLiteral("SHOULD_NOT_COMPLETE")))
    {
        std::cerr << "timed out command unexpectedly completed\n";
        return 6;
    }
    if (runner.isRunning())
    {
        std::cerr << "runner still reports running after timeout\n";
        return 7;
    }

    std::cout << "COMMAND_RUNNER_TIMEOUT_SMOKE_OK\n";
    return 0;
}
