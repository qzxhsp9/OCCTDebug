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

    QString error;
    if (!runner.start(request, &error))
    {
        std::cerr << "failed to start command: " << error.toStdString() << "\n";
        return 1;
    }

    QTimer::singleShot(200, &app, [&]() {
        runner.cancel();
    });
    QTimer::singleShot(6000, &app, [&]() {
        std::cerr << "command cancel timed out\n";
        app.quit();
    });

    app.exec();

    if (!finished)
    {
        return 2;
    }
    if (!result.canceled)
    {
        std::cerr << "result was not marked canceled\n";
        return 3;
    }
    if (result.stdoutText.contains(QStringLiteral("SHOULD_NOT_COMPLETE")))
    {
        std::cerr << "canceled command unexpectedly completed\n";
        return 4;
    }
    if (runner.isRunning())
    {
        std::cerr << "runner still reports running after cancel\n";
        return 5;
    }

    std::cout << "COMMAND_RUNNER_CANCEL_SMOKE_OK\n";
    return 0;
}
