#include "core/runner/CommandRunner.h"

namespace occtdebug
{
CommandRunner::CommandRunner(QObject* parent)
    : QObject(parent)
{
}

CommandRunner::~CommandRunner()
{
    if (m_timeoutTimer != nullptr)
    {
        m_timeoutTimer->stop();
        m_timeoutTimer->deleteLater();
        m_timeoutTimer = nullptr;
    }
    if (m_process != nullptr)
    {
        m_process->kill();
        m_process->waitForFinished(1500);
        m_process->deleteLater();
        m_process = nullptr;
    }
}

bool CommandRunner::isRunning() const
{
    return m_process != nullptr;
}

bool CommandRunner::start(const CommandRequest& request, QString* error)
{
    if (m_process != nullptr)
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("a command is already running");
        }
        return false;
    }

    if (request.program.isEmpty())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("program is empty");
        }
        return false;
    }

    m_result = {};
    m_result.program = request.program;
    m_result.arguments = request.arguments;
    m_result.workingDirectory = request.workingDirectory;
    m_result.timeoutMs = request.timeoutMs;
    m_cancelRequested = false;
    m_timeoutExpired = false;

    m_process = new QProcess(this);
    m_process->setProgram(request.program);
    m_process->setArguments(request.arguments);
    m_process->setProcessEnvironment(request.environment);
    if (!request.workingDirectory.isEmpty())
    {
        m_process->setWorkingDirectory(request.workingDirectory);
    }

    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        const QString text = QString::fromLocal8Bit(m_process->readAllStandardOutput());
        m_result.stdoutText.append(text);
        emit outputReceived(text);
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        const QString text = QString::fromLocal8Bit(m_process->readAllStandardError());
        m_result.stderrText.append(text);
        emit errorOutputReceived(text);
    });
    connect(m_process, &QProcess::finished, this, &CommandRunner::handleFinished);

    if (request.timeoutMs > 0)
    {
        m_timeoutTimer = new QTimer(this);
        m_timeoutTimer->setSingleShot(true);
        connect(m_timeoutTimer, &QTimer::timeout, this, [this]() {
            if (m_process == nullptr)
            {
                return;
            }
            m_timeoutExpired = true;
            m_process->terminate();
            QTimer::singleShot(1500, this, [this]() {
                if (m_process != nullptr && m_timeoutExpired)
                {
                    m_process->kill();
                }
            });
        });
        m_timeoutTimer->start(request.timeoutMs);
    }

    m_elapsed.restart();
    m_process->start();
    if (!m_process->waitForStarted(3000))
    {
        if (error != nullptr)
        {
            *error = m_process->errorString();
        }
        if (m_timeoutTimer != nullptr)
        {
            m_timeoutTimer->stop();
            m_timeoutTimer->deleteLater();
            m_timeoutTimer = nullptr;
        }
        m_process->deleteLater();
        m_process = nullptr;
        m_cancelRequested = false;
        m_timeoutExpired = false;
        return false;
    }

    return true;
}

void CommandRunner::cancel()
{
    if (m_process == nullptr)
    {
        return;
    }

    m_cancelRequested = true;
    m_process->kill();
    QTimer::singleShot(0, this, [this]() {
        if (m_process == nullptr)
        {
            return;
        }
        const bool finished = m_process->state() == QProcess::NotRunning;
        handleFinished(finished ? m_process->exitCode() : -1,
                       finished ? m_process->exitStatus() : QProcess::CrashExit);
    });
}

void CommandRunner::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (m_process == nullptr)
    {
        return;
    }

    m_result.stdoutText.append(QString::fromLocal8Bit(m_process->readAllStandardOutput()));
    m_result.stderrText.append(QString::fromLocal8Bit(m_process->readAllStandardError()));
    m_result.exitCode = exitCode;
    m_result.exitStatus = exitStatus;
    m_result.elapsedMs = m_elapsed.elapsed();
    m_result.canceled = m_cancelRequested;
    m_result.timedOut = m_timeoutExpired;

    if (m_timeoutTimer != nullptr)
    {
        m_timeoutTimer->stop();
        m_timeoutTimer->deleteLater();
        m_timeoutTimer = nullptr;
    }

    QProcess* finishedProcess = m_process;
    m_process = nullptr;
    m_cancelRequested = false;
    m_timeoutExpired = false;
    finishedProcess->deleteLater();

    emit finished(m_result);
}
} // namespace occtdebug
