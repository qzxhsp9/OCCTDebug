#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>
#include <QTimer>

namespace occtdebug
{
struct CommandRequest
{
    QString program;
    QStringList arguments;
    QString workingDirectory;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    int timeoutMs = 0;
};

struct CommandResult
{
    QString program;
    QStringList arguments;
    QString workingDirectory;
    QString stdoutText;
    QString stderrText;
    int exitCode = -1;
    QProcess::ExitStatus exitStatus = QProcess::NormalExit;
    qint64 elapsedMs = 0;
    bool canceled = false;
    bool timedOut = false;
    int timeoutMs = 0;
};

class CommandRunner final : public QObject
{
    Q_OBJECT

public:
    explicit CommandRunner(QObject* parent = nullptr);
    ~CommandRunner() override;

    bool isRunning() const;
    bool start(const CommandRequest& request, QString* error = nullptr);
    void cancel();

signals:
    void outputReceived(const QString& text);
    void errorOutputReceived(const QString& text);
    void finished(const occtdebug::CommandResult& result);

private:
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);

    QProcess* m_process = nullptr;
    QTimer* m_timeoutTimer = nullptr;
    QElapsedTimer m_elapsed;
    CommandResult m_result;
    bool m_cancelRequested = false;
    bool m_timeoutExpired = false;
};
} // namespace occtdebug
