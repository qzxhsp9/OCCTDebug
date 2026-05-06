#include "core/Logger.h"

#include <QDebug>
#include <QMutex>
#include <QStringList>

namespace
{
QMutex g_logMutex;
QStringList g_recent;
constexpr int kMaxRecent = 500;
} // namespace

void Logger::append(const QString& level, const QString& message)
{
    QMutexLocker lock(&g_logMutex);
    const QString line = QStringLiteral("[%1] %2").arg(level, message);
    g_recent.append(line);
    while (g_recent.size() > kMaxRecent)
    {
        g_recent.removeFirst();
    }
    qDebug().noquote() << line;
}

void Logger::info(const QString& message)
{
    append(QStringLiteral("INFO"), message);
}

void Logger::warning(const QString& message)
{
    append(QStringLiteral("WARN"), message);
}

void Logger::error(const QString& message)
{
    append(QStringLiteral("ERROR"), message);
}

QStringList Logger::recentLines()
{
    QMutexLocker lock(&g_logMutex);
    return g_recent;
}

void Logger::clearRecent()
{
    QMutexLocker lock(&g_logMutex);
    g_recent.clear();
}
