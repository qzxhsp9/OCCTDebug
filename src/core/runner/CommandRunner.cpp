#include "core/runner/CommandRunner.h"

namespace occtdebug
{
CommandRunner::CommandRunner(QObject* parent)
    : QObject(parent)
{
}

CommandRunner::~CommandRunner()
{
    cancel();
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

    m_elapsed.restart();
    m_process->start();
    if (!m_process->waitForStarted(3000))
    {
        if (error != nullptr)
        {
            *error = m_process->errorString();
        }
        m_process->deleteLater();
        m_process = nullptr;
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

    m_process->terminate();
    if (!m_process->waitForFinished(1500))
    {
        m_process->kill();
        m_process->waitForFinished(1500);
    }
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

    QProcess* finishedProcess = m_process;
    m_process = nullptr;
    finishedProcess->deleteLater();

    emit finished(m_result);
}
} // namespace occtdebug
